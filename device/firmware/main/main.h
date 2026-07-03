#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "nvs.h"

enum { CORE0, CORE1 };

/***** shared global variables *****/
extern nvs_handle_t nvs;
extern TaskHandle_t led;
extern QueueHandle_t logqueue;
extern QueueHandle_t syslogqueue;
extern QueueHandle_t canlogqueue;
extern QueueHandle_t cantxqueue;
extern esp_mqtt_client_handle_t mqtt;
extern volatile bool file_op_busy;

bool sdcard_log_writer_active(void);

/***** nvs storage *****/
typedef struct {
  struct {
    uint8_t mac[6];
    char macaddr[18];
    char ssid[32];
    char passwd[32];
  } wifi;
  struct {
    char server[64];
    char name[32];
    char key[32];
    char tz[40];
    uint32_t intv;
  } device;
  struct {
    uint8_t can;
    uint8_t gps;
    uint8_t analog;
    uint8_t digital;
  } enabled;
  struct {
    uint8_t bps;
    uint8_t _reserved[3];
    uint32_t filter;
    uint32_t mask;
  } can;
  struct {
    uint8_t dev;
    uint8_t _reserved[3];
  } gps;
} nvs_storage_t;

extern nvs_storage_t storage;

/***** system state *****/
typedef uint32_t state_t;
extern state_t init;
extern const char components[][8];

typedef enum {
  CORE,
  NVS,
  RTC,
  SD,
  WIFI,
  MQTT,
  CAN,
  GPS,
  ANALOG,
  DIGITAL,
  GYRO,
  COMPONENT_MAX = 12,
  COMPONENT_ALL = 0x07FF,
} state_component_t;

#define ALL_ERROR_FATAL (COMPONENT_ALL | (COMPONENT_ALL << COMPONENT_MAX))

typedef enum {
  STATE_OK    = pdMS_TO_TICKS(1000),
  STATE_ERROR = pdMS_TO_TICKS(250),
  STATE_FATAL = pdMS_TO_TICKS(100),
} state_led_interval_t;

static inline void SET_ERROR(state_t *state, state_component_t component) {
  *state |= (1 << component);
  xTaskAbortDelay(led);
}

static inline void SET_FATAL(state_t *state, state_component_t component) {
  *state |= (1 << (component + COMPONENT_MAX));
  xTaskAbortDelay(led);
}

static inline void CLEAR_ERROR(state_t *state, state_component_t component) {
  *state &= ~(1 << component);
  xTaskAbortDelay(led);
}

static inline void CLEAR_FATAL(state_t *state, state_component_t component) {
  *state &= ~(1 << (component + COMPONENT_MAX));
  xTaskAbortDelay(led);
}

static inline void CLEAR_ALL(state_t *state, state_component_t component) {
  *state &= ~((1 << component) | (1 << (component + COMPONENT_MAX)));
  xTaskAbortDelay(led);
}

static inline void COPY_STATE(state_t *dest, state_t *src, state_component_t component) {
  *dest = (*dest & ~(1 << component)) | (*src & (1 << component));
  *dest = (*dest & ~(1 << (component + COMPONENT_MAX))) | (*src & (1 << (component + COMPONENT_MAX)));
  xTaskAbortDelay(led);
}

#define IS_ERROR(state, component) (*state & (1 << component))
#define IS_FATAL(state, component) (*state & (1 << (component + COMPONENT_MAX)))
#define IS_OK(state, component) (!IS_ERROR(state, component) && !IS_FATAL(state, component))

/***** log protocol *****/
#define PROTOCOL_VERSION 1
#define LOG_MAGIC 0xAE

typedef enum {
  LOG_TYPE_INVALID,
  LOG_TYPE_BOOT,
  LOG_TYPE_CAN,
  LOG_TYPE_GPS,
  LOG_TYPE_ANALOG,
  LOG_TYPE_DIGITAL,
  LOG_TYPE_GYROSCOPE,
  LOG_TYPE_SYSTEM,
  LOG_TYPE_USER_EVENT,
} log_type_t;

typedef struct {
  uint8_t protocol_version;
  uint8_t _reserved[1];
  uint8_t mac[6];
  uint64_t boot_time;  // second since epoch
} boot_record_t;

typedef struct {
  uint32_t id;
  uint8_t extended;
  uint8_t remote;
  uint8_t len;
  uint8_t _reserved[1];
  uint8_t data[8];
} can_record_t;

typedef struct {
  uint32_t latitude;
  uint32_t longitude;
  uint8_t lat_dir;
  uint8_t lon_dir;
  uint8_t _reserved[2];
  uint16_t speed;
  uint16_t course;
} gps_record_t;

typedef struct {
  int16_t ain1;
  int16_t ain2;
  int16_t ain3;
  int16_t ain4;
  int16_t ain5;
  int16_t ain6;
  int16_t voltage;
  int16_t temperature;
} analog_record_t;

typedef struct {
  uint32_t din1;
  uint32_t din2;
  uint32_t din3;
  uint32_t din4;
} digital_record_t;

typedef struct {
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t temperature;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
  uint8_t _reserved[2];
} gyroscope_record_t;

typedef struct {
  char msg[16];
} system_event_t;

typedef system_event_t user_event_t;

typedef struct {
  uint8_t magic;
  uint8_t type;
  uint16_t checksum;
  uint32_t timestamp;
  union {
    boot_record_t boot;
    can_record_t can;
    gps_record_t gps;
    analog_record_t analog;
    digital_record_t digital;
    gyroscope_record_t gyroscope;
    system_event_t system_event;
    user_event_t user_event;
  } payload;  // 16 bytes
} log_t;

typedef struct {
  uint32_t timestamp;
  state_t run;
  log_t gps;
  log_t gyro;
  log_t analog;
  log_t digital;
} log_buf_t;

extern log_buf_t logbuf;

static inline void log_prepare(uint8_t type, log_t *log) {
  uint32_t *ptr   = (uint32_t *)log;
  uint32_t chksum = 0;

  // set log header
  log->magic     = LOG_MAGIC;
  log->type      = type;
  log->checksum  = 0;
  log->timestamp = (uint32_t)(esp_timer_get_time() / 1000);

  // calculate checksum
  for (size_t i = 0; i < sizeof(log_t) / sizeof(uint32_t); i++) {
    chksum ^= ptr[i];
  }

  // fold to 16 bit
  log->checksum = (chksum & 0xFFFF) + (chksum >> 16);
}

static inline int LOG(uint8_t type, log_t *log) {
  log_prepare(type, log);
  if (logqueue == NULL) return 0;
  return xQueueSend(logqueue, log, 0);
}

static inline int LOG_FROM_ISR(uint8_t type, log_t *log) {
  log_prepare(type, log);
  if (logqueue == NULL) return 0;
  return xQueueSendFromISR(logqueue, log, NULL);
}

static inline void SYSLOG(const char *msg) {
  if (logqueue == NULL && syslogqueue == NULL) return;
  log_t log = { 0 };
  strncpy(log.payload.system_event.msg, msg, sizeof(log.payload.system_event.msg));  // no need to null-terminate
  log_prepare(LOG_TYPE_SYSTEM, &log);
  if (logqueue != NULL) xQueueSend(logqueue, &log, 0);
  if (syslogqueue != NULL) xQueueSend(syslogqueue, &log, 0);
}

static inline void ERROR_LOG(state_t *state, state_component_t component, const char *msg) {
  SET_ERROR(state, component);
  ESP_LOGW(components[component], "%s", msg);
}

static inline void FATAL_LOG(state_t *state, state_component_t component, const char *msg) {
  SET_FATAL(state, component);
  ESP_LOGE(components[component], "%s", msg);
}

static inline void ERROR_SYSLOG(state_t *state, state_component_t component, const char *msg, const char *log) {
  SYSLOG(log);
  ERROR_LOG(state, component, msg);
}

static inline void FATAL_SYSLOG(state_t *state, state_component_t component, const char *msg, const char *log) {
  SYSLOG(log);
  FATAL_LOG(state, component, msg);
}

/***** utility functions *****/
#define INFO(component, fmt, ...) ESP_LOGI(components[component], fmt, ##__VA_ARGS__)

static inline int BCD_TO_DEC(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }
static inline uint8_t DEC_TO_BCD(int dec) { return ((dec / 10) << 4) | (dec % 10); }

/***** peripheral configs *****/
enum {
  MQTT_QOS_0,
  MQTT_QOS_1,
  MQTT_QOS_2,
};

enum {
  CAN_BPS_1K,
  CAN_BPS_5K,
  CAN_BPS_10K,
  CAN_BPS_12_5K,
  CAN_BPS_16K,
  CAN_BPS_20K,
  CAN_BPS_25K,
  CAN_BPS_50K,
  CAN_BPS_100K,
  CAN_BPS_125K,
  CAN_BPS_250K,
  CAN_BPS_500K,
  CAN_BPS_800K,
  CAN_BPS_1M,
  CAN_BPS_MAX
};

enum {
  GPS_DEV_UBLOX,
  GPS_DEV_MAX,
};

/***** I2C timeout *****/
#define I2C_TIMEOUT_MS 10

/***** sensor task intervals *****/
#define TASK_INTERVAL_GYRO   pdMS_TO_TICKS(10)   // 100Hz

#ifndef CONFIG_MONOLITH_MINI
#define TASK_INTERVAL_ANALOG pdMS_TO_TICKS(10)   // 100Hz (7ch ADS1115, ~6.2ms/cycle with pipelining)
#else
#define TASK_INTERVAL_ANALOG pdMS_TO_TICKS(100)  // 10Hz (온도+전압만, 고속 불필요)
#endif

#endif  // MAIN_H
