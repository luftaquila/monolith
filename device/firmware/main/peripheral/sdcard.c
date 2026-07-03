#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "main.h"

#include "driver/sdmmc_host.h"
#include "driver/twai.h"
#include "esp_vfs_fat.h"

extern struct timeval boot;

char logpath[64];

static bool queue_init(void) {
  if (logqueue == NULL) {
    logqueue = xQueueCreate(2560, sizeof(log_t));
  }

  if (syslogqueue == NULL) {
    syslogqueue = xQueueCreate(32, sizeof(log_t));
  }

  if (canlogqueue == NULL) {
    canlogqueue = xQueueCreate(1024, sizeof(log_t));
  }

  if (cantxqueue == NULL) {
    cantxqueue = xQueueCreate(4, sizeof(twai_message_t));
  }

  return logqueue != NULL && syslogqueue != NULL && canlogqueue != NULL && cantxqueue != NULL;
}

/*******************************************************************************
 * save log queue to SD card every 1000 ms
 ******************************************************************************/
static void task_sdcard(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();

  int ret;
  int fd = (int)pvParameters;
  log_t log;
  int write_count = 0;
  int cycle_count = 0;

  while (true) {
    if (file_op_busy) {
      if (write_count > 0) {
        fsync(fd);
        write_count = 0;
        cycle_count = 0;
      }
      xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
      continue;
    }

    do {
      if ((ret = xQueueReceive(logqueue, &log, 0)) == pdTRUE) {
        write(fd, &log, sizeof(log));
        write_count++;
      }
    } while (ret);

    cycle_count++;

    if (write_count > 0 && (write_count >= 512 || cycle_count >= 3)) {
      if (fsync(fd) != 0 && !IS_FATAL(&logbuf.run, SD)) {
        FATAL_LOG(&logbuf.run, SD, "fsync failure");
      }
      write_count = 0;
      cycle_count = 0;
      INFO(SD, "log sync");
    }

    xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
  }
}

/*******************************************************************************
 * init SDIO, mount filesystem and create task
 ******************************************************************************/
void sdcard_init(void) {
  if (queue_init() != true) {
    FATAL_LOG(&init, SD, "queue create failure");
    goto finish;
  }

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed   = false,
    .max_files                = 4,
    .allocation_unit_size     = 32 * 512,
    .disk_status_check_enable = false,
    .use_one_fat              = false,
  };

  sdmmc_card_t *card;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  sdmmc_slot_config_t slot_config = {
    .clk   = GPIO_NUM_39,
    .cmd   = GPIO_NUM_40,
    .d0    = GPIO_NUM_38,
    .d1    = GPIO_NUM_37,
    .d2    = GPIO_NUM_42,
    .d3    = GPIO_NUM_41,
    .cd    = SDMMC_SLOT_NO_CD,
    .wp    = SDMMC_SLOT_NO_WP,
    .width = 4,
    .flags = 0,
  };

  if (esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card) != ESP_OK) {
    FATAL_LOG(&init, SD, "mount failure");
    goto finish;
  }

  // set log file
  setenv("TZ", storage.device.tz, 1);
  tzset();

  struct tm tp;
  struct tm *tm = localtime_r(&boot.tv_sec, &tp);

  strftime(logpath, sizeof(logpath), "/sdcard/%Y-%m-%d-%H-%M-%S.log", tm);

  setenv("TZ", "UTC", 1);
  tzset();

  int fd = open(logpath, O_RDWR | O_CREAT | O_TRUNC, 0);

  if (fd < 0) {
    FATAL_LOG(&init, SD, "file open failure");
    goto finish;
  }

  if (xTaskCreate(task_sdcard, "sdcard", 4096, (void *)fd, 7, NULL) != pdPASS) {
    FATAL_LOG(&init, SD, "task create failure");
    close(fd);
    goto finish;
  }

  INFO(SD, "log file: %s", logpath);

  log_t boot_record;
  boot_record.payload.boot.protocol_version = PROTOCOL_VERSION;
  boot_record.payload.boot.boot_time        = (uint64_t)boot.tv_sec;
  memcpy(boot_record.payload.boot.mac, storage.wifi.mac, sizeof(storage.wifi.mac));

  if (LOG(LOG_TYPE_BOOT, &boot_record) != true) {
    FATAL_LOG(&init, SD, "boot record failure");
    goto finish;
  }

finish:
  if (IS_OK(&init, SD)) {
    CLEAR_ALL(&logbuf.run, SD);
  } else {
    COPY_STATE(&logbuf.run, &init, SD);
  }
}
