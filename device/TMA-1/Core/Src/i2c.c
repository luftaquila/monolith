/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
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
#include "i2c.h"

/* USER CODE BEGIN 0 */
extern LOG syslog;

// I2C tx buffers
extern uint32_t i2c_flag;

extern ring_buffer_t TELEMETRY_BUFFER;
uint8_t TELEMETRY_BUFFER_ARR[1 << 15]; // 32KB

// accelerometer data
extern uint8_t acc_value[6];

// ESP32 TELEMETRY tx interrupt callback
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    if (ring_buffer_is_empty(&TELEMETRY_BUFFER)) {
      // finish transmission
      i2c_flag &= ~(1 << I2C_BUFFER_TELEMETRY_REMAIN);
      i2c_flag &= ~(1 << I2C_BUFFER_TELEMETRY_TRANSMIT);
    }
    else {
      static uint8_t payload[sizeof(LOG)];
      ring_buffer_dequeue_arr(&TELEMETRY_BUFFER, (char *)payload, sizeof(LOG));
      HAL_I2C_Master_Transmit_IT(&hi2c1, ESP_I2C_ADDR, payload, sizeof(LOG));
    }
  }

  return;
}

// ADXL345 ACCELEROMETER mem read interrupt callback
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  *(uint64_t *)syslog.value = *(uint64_t *)acc_value;
  SYS_LOG(LOG_INFO, ACCELEROMETER, ACCELEROMETER_DATA);

  return;
}


/****************************
 * ESP32 I2C interface
 ***************************/
int TELEMETRY_SETUP(void) {
  // initialize buffer
  ring_buffer_init(&TELEMETRY_BUFFER, (char *)TELEMETRY_BUFFER_ARR, sizeof(TELEMETRY_BUFFER_ARR));

  // ESP handshake process
  HAL_Delay(1000);
  HAL_I2C_Master_Transmit(&hi2c1, ESP_I2C_ADDR, (uint8_t *)"READY", 5, 500);

  // receive ACK from UART (10 bytes)
  uint8_t ack[10];
  for (int32_t i = 0; i < 10; i++) {
    HAL_UART_Receive(&huart2, (ack + i), 1, 10);
  }

  if (strstr((char *)ack, "ACK") == NULL) {
    goto esp_fail;
  }

  // waiting for time sync (10 sec)
  uint8_t esp_rtc_fix[25];
  if (HAL_UART_Receive(&huart2, esp_rtc_fix, 25, 10000) != HAL_OK) {
    goto esp_fail;
  }

  // example: $ESP 2023-06-05-22-59-38
  if (strncmp((char *)esp_rtc_fix, "$ESP ", 5) == 0) {
    DEBUG_MSG("[%8lu] [OK ] ESP TIME SYNC: %.*s\r\n", HAL_GetTick(), 24, esp_rtc_fix);

    SYS_LOG(LOG_INFO, SYS, SYS_TELEMETRY_REMOTE);

    // set RTC
    uint8_t *ptr = esp_rtc_fix + 7;
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
    SYS_LOG(LOG_INFO, SYS, SYS_TELEMETRY_RTC_FIX);

    return SYS_OK;
  }

esp_fail:
  return SYS_ERROR;
}
/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}
/* I2C3 init function */
void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 400000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = I2C_ESP_SCL_Pin|I2C_ESP_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
  else if(i2cHandle->Instance==I2C3)
  {
  /* USER CODE BEGIN I2C3_MspInit 0 */

  /* USER CODE END I2C3_MspInit 0 */

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**I2C3 GPIO Configuration
    PC9     ------> I2C3_SDA
    PA8     ------> I2C3_SCL
    */
    GPIO_InitStruct.Pin = I2C_ACC_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(I2C_ACC_SDA_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = I2C_ACC_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(I2C_ACC_SCL_GPIO_Port, &GPIO_InitStruct);

    /* I2C3 clock enable */
    __HAL_RCC_I2C3_CLK_ENABLE();

    /* I2C3 interrupt Init */
    HAL_NVIC_SetPriority(I2C3_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
  /* USER CODE BEGIN I2C3_MspInit 1 */

  /* USER CODE END I2C3_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(I2C_ESP_SCL_GPIO_Port, I2C_ESP_SCL_Pin);

    HAL_GPIO_DeInit(I2C_ESP_SDA_GPIO_Port, I2C_ESP_SDA_Pin);

    /* I2C1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
  else if(i2cHandle->Instance==I2C3)
  {
  /* USER CODE BEGIN I2C3_MspDeInit 0 */

  /* USER CODE END I2C3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C3_CLK_DISABLE();

    /**I2C3 GPIO Configuration
    PC9     ------> I2C3_SDA
    PA8     ------> I2C3_SCL
    */
    HAL_GPIO_DeInit(I2C_ACC_SDA_GPIO_Port, I2C_ACC_SDA_Pin);

    HAL_GPIO_DeInit(I2C_ACC_SCL_GPIO_Port, I2C_ACC_SCL_Pin);

    /* I2C3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(I2C3_EV_IRQn);
  /* USER CODE BEGIN I2C3_MspDeInit 1 */

  /* USER CODE END I2C3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
