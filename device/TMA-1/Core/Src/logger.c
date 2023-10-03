/*
 * logger.c
 *
 *  Created on: Sep 10, 2023
 *      Author: LUFT-AQUILA
 */

#include "sdio.h"
#include "debug.h"
#include "logger.h"
#include "ringbuffer.h"

extern LOG syslog;
extern SYSTEM_STATE sys_state;
extern ring_buffer_t SD_BUFFER;

#ifdef ENABLE_TELEMETRY
extern uint32_t telemetry_flag;
extern uint32_t handshake_flag;
extern ring_buffer_t TELEMETRY_BUFFER;
#endif

#ifdef ENABLE_SERIAL
extern uint32_t serial_flag;
extern ring_buffer_t SERIAL_BUFFER;
#endif

int SYS_LOG(LOG_LEVEL level, LOG_SOURCE source, int key) {
  syslog.timestamp = HAL_GetTick();
  syslog.level = level;
  syslog.source = source;
  syslog.key = (uint8_t)(key & 0xff);

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

#ifdef ENABLE_TELEMETRY
  if (sys_state.TELEMETRY && (handshake_flag & (1 << REMOTE_CONNECTED))) {
    telemetry_flag |= 1 << TELEMETRY_BUFFER_REMAIN;
    ring_buffer_queue_arr(&TELEMETRY_BUFFER, (char *)&syslog, sizeof(LOG));
  }
#endif

#ifdef ENABLE_SERIAL
  serial_flag |= 1 << SERIAL_BUFFER_REMAIN;
  ring_buffer_queue_arr(&SERIAL_BUFFER, (char *)&syslog, sizeof(LOG));
#endif

#ifdef DEBUG_MODE
  if (syslog.source == CAN) {
    DEBUG_MSG("[%8lu] [LOG] %s\t%s\t%03x\t\t%02x %02x %02x %02x %02x %02x %02x %02x\r\n", syslog.timestamp, STR_LOG_LEVEL[syslog.level], STR_LOG_SOURCE[syslog.source], syslog.key, syslog.value[0], syslog.value[1], syslog.value[2], syslog.value[3], syslog.value[4], syslog.value[5], syslog.value[6], syslog.value[7]);
  } else {
    DEBUG_MSG("[%8lu] [LOG] %s\t%s\t%s\t\t%02x %02x %02x %02x %02x %02x %02x %02x\r\n", syslog.timestamp, STR_LOG_LEVEL[syslog.level], STR_LOG_SOURCE[syslog.source], STR_LOG_KEY[syslog.source][syslog.key], syslog.value[0], syslog.value[1], syslog.value[2], syslog.value[3], syslog.value[4], syslog.value[5], syslog.value[6], syslog.value[7]);
  }
#endif

  // reset log value
  *(uint64_t *)syslog.value = 0;

  return 0;
}
