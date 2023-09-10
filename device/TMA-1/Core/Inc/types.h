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
  TIMER_100ms = 0,
  TIMER_400ms,
  TIMER_1s,
} TIMER_ID;

/* I2C buffer index */
typedef enum {
  I2C_BUFFER_TELEMETRY_REMAIN = 0,
  I2C_BUFFER_TELEMETRY_TRANSMIT,
  I2C_BUFFER_LOG_REMAIN,
  I2C_BUFFER_LOG_TRANSMIT,
} I2C_BUFFER_ID;

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

#endif /* CORE_INC_TYPES_H_ */
