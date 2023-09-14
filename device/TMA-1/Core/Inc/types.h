/*
 * types.h
 *
 *  Created on: Sep 10, 2023
 *      Author: LUFT-AQUILA
 */

#ifndef CORE_INC_TYPES_H_
#define CORE_INC_TYPES_H_

#define ADC_COUNT 4
#define PULSE_CH_COUNT 4

#define I2C_TELEMETRY     &hi2c1
#define I2C_ACCELEROMETER &hi2c3

#define UART_DEBUG     &huart1
#define UART_TELEMETRY &huart2
#define UART_GPS       &huart3
#define UART_SERIAL    &huart6

/* RTC datetime */
typedef struct {
  uint8_t year;
  uint8_t month;
  uint8_t date;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} DATETIME;

/* timer index */
typedef enum {
  FLAG_TIMER_100ms = 0,
  FLAG_TIMER_500ms,
  FLAG_TIMER_1s,
} TIMER_FLAG;

/* telemetry buffer */
typedef enum {
  TELEMETRY_BUFFER_REMAIN = 0,
  TELEMETRY_BUFFER_TRANSMIT,
} TELEMETRY_BUFFER_FLAG;

/* UART log output buffer */
typedef enum {
  SERIAL_BUFFER_REMAIN = 0,
  SERIAL_BUFFER_TRANSMIT,
} SERIAL_BUFFER_FLAG;

/* NMEA GPRMC message */
typedef struct nmea_gprmc_t {
  uint8_t *id;
  uint8_t *utc_time;
  uint8_t *status;
  uint8_t *lat;
  uint8_t *north;
  uint8_t *lon;
  uint8_t *east;
  uint8_t *speed;
  uint8_t *course;
  uint8_t *utc_date;
  uint8_t *others;
} NMEA_GPRMC;

typedef struct {
  uint32_t lat;
  uint32_t lon;
} GPS_COORD;

typedef struct {
  uint32_t speed;
  uint32_t course;
} GPS_VECTOR;

typedef struct {
  uint32_t utc_date;
  uint32_t utc_time;
} GPS_DATETIME;

/* system state type */
typedef struct {
  uint8_t ERR;
  uint8_t SD;
  uint8_t TELEMETRY;
  uint8_t CAN;
} SYSTEM_STATE;

typedef enum {
  SYS_OK = 0,
  SYS_ERROR
} SYS_STAUS;

#endif /* CORE_INC_TYPES_H_ */
