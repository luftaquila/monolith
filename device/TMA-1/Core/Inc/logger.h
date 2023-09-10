/*
 * logger.h
 *
 *  Created on: Sep 10, 2023
 *      Author: LUFT-AQUILA
 */

#ifndef CORE_INC_LOGGER_H_
#define CORE_INC_LOGGER_H_

/* system log structure */
typedef struct {
  uint32_t timestamp;
  uint8_t level;
  uint8_t source;
  uint8_t key;
  uint8_t checksum;
  uint8_t value[8];
} LOG;

/* log level type */
typedef enum {
  LOG_FATAL = 0,
  LOG_ERROR,
  LOG_WARN,
  LOG_INFO,
  LOG_DEBUG,
} LOG_LEVEL;

/* log source types */
typedef enum {
  SYS = 0,
  CAN,
  DIGITAL,
  ANALOG,
  PULSE,
  ACCELEROMETER,
  GPS,
  TELEMETRY
} LOG_SOURCE;

/* log key types per sources */
typedef enum {
  SYS_BOOT = 0,
  SYS_STATE,
  SYS_READY,
  SD_INIT
} LOG_KEY_SYS;

typedef enum {
  CAN_INIT = 0,
  CAN_ERR
} LOG_KEY_CAN;

typedef enum {
  DIGITAL_INIT = 0,
  DIGITAL_DATA
} LOG_KEY_DIGITAL;

typedef enum {
  ANALOG_INIT = 0,
  ANALOG_SYS,
  ANALOG_DATA
} LOG_KEY_ANALOG;

typedef enum {
  PULSE_INIT = 0,
  PULSE_DATA
} LOG_KEY_PULSE;

typedef enum {
  ACCELEROMETER_INIT = 0,
  ACCELEROMETER_DATA
} LOG_KEY_ACCELEROMETER;

typedef enum {
  GPS_INIT = 0,
  GPS_POS,
  GPS_VEC,
  GPS_TIME
} LOG_KEY_GPS;

typedef enum {
  TELEMETRY_INIT = 0,
  TELEMETRY_REMOTE_CONNECTION,
  TELEMETRY_RTC_FIX
} LOG_KEY_TELEMETRY;


/* system state type */
typedef struct {
  uint8_t SD;
  uint8_t CAN;
  uint8_t TELEMETRY;
  uint8_t ACCELEROMETER;
  uint8_t GPS;

  uint8_t _reserved[3];
} SYSTEM_STATE;

/* Prototypes */
int SYS_LOG(LOG_LEVEL level, LOG_SOURCE source, int key);

#endif /* CORE_INC_LOGGER_H_ */
