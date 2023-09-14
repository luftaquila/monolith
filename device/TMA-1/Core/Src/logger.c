/*
 * logger.c
 *
 *  Created on: Sep 10, 2023
 *      Author: LUFT-AQUILA
 */

#include "sdio.h"
#include "logger.h"
#include "ringbuffer.h"

extern LOG syslog;
extern SYSTEM_STATE sys_state;
extern ring_buffer_t SD_BUFFER;

extern uint32_t telemetry_flag;
extern ring_buffer_t TELEMETRY_BUFFER;

extern uint32_t serial_flag;
extern ring_buffer_t SERIAL_BUFFER;

int SYS_LOG(LOG_LEVEL level, LOG_SOURCE source, int key) {
  syslog.timestamp = HAL_GetTick();
  syslog.level = level;
  syslog.source = source;
  syslog.key = key;

  uint32_t sum = *(uint8_t *)(&syslog);
  sum += *((uint8_t *)(&syslog) + 1);
  sum += *((uint8_t *)(&syslog) + 2);
  sum += *((uint8_t *)(&syslog) + 3);
  sum += *((uint8_t *)(&syslog) + 4);
  sum += *((uint8_t *)(&syslog) + 5);
  sum += *((uint8_t *)(&syslog) + 6);
  sum += *((uint8_t *)(&syslog) + 8);
  sum += *((uint8_t *)(&syslog) + 9);
  sum += *((uint8_t *)(&syslog) + 10);
  sum += *((uint8_t *)(&syslog) + 11);
  sum += *((uint8_t *)(&syslog) + 12);
  sum += *((uint8_t *)(&syslog) + 13);
  sum += *((uint8_t *)(&syslog) + 14);
  sum += *((uint8_t *)(&syslog) + 15);

  syslog.checksum = (uint8_t)(sum & 0xff);

  if (sys_state.SD) {
    ring_buffer_queue_arr(&SD_BUFFER, (char *)&syslog, sizeof(LOG));
  }

  if (sys_state.TELEMETRY) {
    telemetry_flag |= 1 << TELEMETRY_BUFFER_REMAIN;
    ring_buffer_queue_arr(&TELEMETRY_BUFFER, (char *)&syslog, sizeof(LOG));
  }

#ifdef ENABLE_SERIAL
  serial_flag |= 1 << SERIAL_BUFFER_REMAIN;
  ring_buffer_queue_arr(&SERIAL_BUFFER, (char *)&syslog, sizeof(LOG));
#endif

  // reset log value
  *(uint64_t *)syslog.value = 0;

  return 0;
}
