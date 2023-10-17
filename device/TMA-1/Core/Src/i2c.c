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

#ifdef ENABLE_TELEMETRY
extern uint32_t telemetry_flag;
extern uint32_t handshake_flag;

extern ring_buffer_t TELEMETRY_BUFFER;
extern uint8_t TELEMETRY_BUFFER_ARR[1 << 14];
#endif

// accelerometer data
#ifdef ENABLE_MONITOR_ACCELEROMETER
extern uint32_t accelerometer_flag;
extern uint8_t accelerometer_value[6];
#endif

// ESP32 TELEMETRY tx interrupt callback
#ifdef ENABLE_TELEMETRY
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    if (ring_buffer_is_empty(&TELEMETRY_BUFFER)) {
      // finish transmission
      telemetry_flag &= ~(1 << TELEMETRY_BUFFER_REMAIN);
      telemetry_flag &= ~(1 << TELEMETRY_BUFFER_TRANSMIT);
    } else {
      static uint8_t payload[sizeof(LOG)];
      ring_buffer_dequeue_arr(&TELEMETRY_BUFFER, (char *)payload, sizeof(LOG));
      HAL_I2C_Master_Transmit_IT(I2C_TELEMETRY, ESP_I2C_ADDR, payload, sizeof(LOG));
    }
  }

  return;
}

// ESP32 RTC fix receive interrupt callback
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    handshake_flag |= (1 << RTC_FIXED);
    RTC_FIX(RTC_ESP);
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
  
}
#endif

/* ADXL345 accelerometer memory read */
#ifdef ENABLE_MONITOR_ACCELEROMETER
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  accelerometer_flag = true;
}
#endif


/****************************
 * ESP32 I2C interface
 ***************************/
#ifdef ENABLE_TELEMETRY
int TELEMETRY_SETUP(void) {
  // initialize buffer
  ring_buffer_init(&TELEMETRY_BUFFER, (char *)TELEMETRY_BUFFER_ARR, sizeof(TELEMETRY_BUFFER_ARR));

  HAL_Delay(500);

  // polling ESP boot (ESP_COMM == HIGH)
  uint32_t start_time = HAL_GetTick();
  while (HAL_GPIO_ReadPin(GPIOB, ESP_COMM_Pin) == GPIO_PIN_RESET) {
    if (HAL_GetTick() > start_time + 3000) {
      DEBUG_MSG("[%8lu] [ERR] ESP not found\r\n", HAL_GetTick());
      return ESP_NOT_FOUND; // 3s timeout
    }
  }

  // handshake call
  int ret = HAL_I2C_IsDeviceReady(I2C_TELEMETRY, ESP_I2C_ADDR, 5, 100);

  if (ret != 0) {
    DEBUG_MSG("[%8lu] [ERR] ESP i2c not ready %u/%u\r\n", HAL_GetTick(), ret, HAL_I2C_GetError(I2C_TELEMETRY));
    return ESP_I2C_NOT_READY;
  }

  ret = HAL_I2C_Master_Transmit(I2C_TELEMETRY, ESP_I2C_ADDR, (uint8_t *)"READY", 5, 100);

  if (ret != 0) {
    DEBUG_MSG("[%8lu] [ERR] ESP handshake error %u\r\n", HAL_GetTick(), HAL_I2C_GetError(I2C_TELEMETRY));
    return ESP_HANDSHAKE_ERR;
  }

  // ESP should set ESP_COMM to LOW after handshake
  while (HAL_GPIO_ReadPin(GPIOB, ESP_COMM_Pin) == GPIO_PIN_SET) {
    if (HAL_GetTick() > start_time + 3000) {
      DEBUG_MSG("[%8lu] [ERR] ESP handshaking process totally ruined\r\n", HAL_GetTick());
      return ESP_HANDSHAKE_RUINED; // 3s timeout
    }
  }

  handshake_flag |= (1 << HANDSHAKE_FINISHED);
  return SYS_OK;
}


void TELEMETRY_TRANSMIT_LOG(void) {
  if (telemetry_flag & (1 << TELEMETRY_BUFFER_REMAIN) && !(telemetry_flag & (1 << TELEMETRY_BUFFER_TRANSMIT))) {
    static uint8_t payload[sizeof(LOG)];
    ring_buffer_dequeue_arr(&TELEMETRY_BUFFER, (char *)payload, sizeof(LOG));
    int ret = HAL_I2C_Master_Transmit_IT(I2C_TELEMETRY, ESP_I2C_ADDR, payload, sizeof(LOG));

    if (ret == HAL_OK) {
      telemetry_flag |= 1 << TELEMETRY_BUFFER_TRANSMIT;
    } else {
      ring_buffer_queue_arr(&TELEMETRY_BUFFER, (char *)payload, sizeof(LOG));
    }
  }
}
#endif


/****************************************
 * ADXL345 accelerometer I2C interface
 ***************************************/
#ifdef ENABLE_MONITOR_ACCELEROMETER
int ACCELEROMETER_WRITE(uint8_t reg, uint8_t value) {
  uint8_t payload[2] = { reg, value };
  return HAL_I2C_Master_Transmit(&hi2c3, ACC_I2C_ADDR, payload, 2, 50);
}

int ACCELEROMETER_SETUP(void) {
  int ret = 0;

  ret |= ACCELEROMETER_WRITE(0x31, 0x01);  // DATA_FORMAT range +-4g
  ret |= ACCELEROMETER_WRITE(0x2D, 0x00);  // POWER_CTL bit reset
  ret |= ACCELEROMETER_WRITE(0x2D, 0x08);  // POWER_CTL set measure mode. 100hz default rate

  return ret;
}
#endif
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
