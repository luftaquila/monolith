/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */
extern LOG syslog;

extern uint8_t rtc[25];

void RTC_FIX(int source) {
  uint8_t *ptr = rtc + 2;
  uint8_t tmp[3];
  int32_t cnt = 0;

  RTC_DateTypeDef RTC_DATE;
  RTC_TimeTypeDef RTC_TIME;

  while (*ptr && cnt < 6) {
    strncpy((char *)tmp, (char *)ptr, 3);
    tmp[2] = '\0';

    switch (cnt) {
      case 0: RTC_DATE.Year    = (uint8_t)strtol((char *)tmp, NULL, 10); break;
      case 1: RTC_DATE.Month   = (uint8_t)strtol((char *)tmp, NULL, 16); break;
      case 2: RTC_DATE.Date    = (uint8_t)strtol((char *)tmp, NULL, 10); break;
      case 3: RTC_TIME.Hours   = (uint8_t)strtol((char *)tmp, NULL, 10); break;
      case 4: RTC_TIME.Minutes = (uint8_t)strtol((char *)tmp, NULL, 10); break;
      case 5: RTC_TIME.Seconds = (uint8_t)strtol((char *)tmp, NULL, 10); break;
    }

    // move to next datetime
    ptr += 3;
    cnt++;
  }

  // set weekday; required for accurate year value
  RTC_DATE.WeekDay = 0;

  HAL_RTC_SetTime(&hrtc, &RTC_TIME, FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &RTC_DATE, FORMAT_BIN);

  syslog.value[0] = RTC_DATE.Year;
  syslog.value[1] = RTC_DATE.Month;
  syslog.value[2] = RTC_DATE.Date;
  syslog.value[3] = RTC_TIME.Hours;
  syslog.value[4] = RTC_TIME.Minutes;
  syslog.value[5] = RTC_TIME.Seconds;
  syslog.value[6] = source;
  SYS_LOG(LOG_INFO, SYS, SYS_RTC_FIX);

  DEBUG_MSG("[%8lu] [ OK] RTC fixed by %s: %.*s\r\n", HAL_GetTick(), source ? "ESP" : "UART", 19, rtc);

  return;
}

void RTC_READ(DATETIME *boot) {
  RTC_DateTypeDef RTC_DATE;
  RTC_TimeTypeDef RTC_TIME;

  HAL_RTC_GetTime(&hrtc, &RTC_TIME, FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &RTC_DATE, FORMAT_BIN);

  boot->second = RTC_TIME.Seconds;
  boot->minute = RTC_TIME.Minutes;
  boot->hour = RTC_TIME.Hours;
  boot->date = RTC_DATE.Date;
  boot->month = RTC_DATE.Month;
  boot->year = RTC_DATE.Year;

  return;
}
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == 0xCAFE) {
    return;
  }

  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0xCAFE);
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_FRIDAY;
  sDate.Month = RTC_MONTH_MAY;
  sDate.Date = 0x12;
  sDate.Year = 0x23;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
