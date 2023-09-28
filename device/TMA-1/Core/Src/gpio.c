/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
#include "gpio.h"

/* USER CODE BEGIN 0 */
extern LOG syslog;

extern uint8_t rtc[25];

#ifdef ENABLE_TELEMETRY
extern uint32_t handshake_flag;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  static uint32_t rtc_fix_triggered = false;
  if ((GPIO_Pin == GPIO_PIN_8) && (handshake_flag & (1 << HANDSHAKE_FINISHED))) {

    if (HAL_GPIO_ReadPin(GPIOB, ESP_COMM_Pin)) {
      handshake_flag |= (1 << REMOTE_CONNECTED);

      syslog.value[0] = true;
      SYS_LOG(LOG_INFO, SYS, SYS_TELEMETRY_REMOTE);
      HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_SET);

      DEBUG_MSG("[%8lu] [INF] remote telemetry server connected\r\n", HAL_GetTick());

      if (!rtc_fix_triggered && !(handshake_flag & (1 << RTC_FIXED))) {
        rtc_fix_triggered = true;
        HAL_I2C_Master_Receive_IT(I2C_TELEMETRY, ESP_I2C_ADDR, rtc, sizeof(rtc));
      }
    } else {
      handshake_flag &= ~(1 << REMOTE_CONNECTED);

      syslog.value[0] = false;
      SYS_LOG(LOG_WARN, SYS, SYS_TELEMETRY_REMOTE);
      HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_RESET);

      DEBUG_MSG("[%8lu] [INF] remote telemetry server disconnected\r\n", HAL_GetTick());
    }
  }
}
#endif

#ifdef ENABLE_MONITOR_DIGITAL
int DIGITAL_SETUP(void) {
  return SYS_OK;
}
#endif

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED_ONBOARD_0_Pin|LED_ONBOARD_1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LED_CUSTOM_0_Pin|LED_CUSTOM_1_Pin|LED_ERR_Pin|LED_HEARTBEAT_Pin
                          |LED_SD_Pin|LED_CAN_Pin|LED_TELEMETRY_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PAPin PAPin */
  GPIO_InitStruct.Pin = LED_ONBOARD_0_Pin|LED_ONBOARD_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PEPin PEPin PEPin PEPin
                           PEPin PEPin PEPin */
  GPIO_InitStruct.Pin = LED_CUSTOM_0_Pin|LED_CUSTOM_1_Pin|LED_ERR_Pin|LED_HEARTBEAT_Pin
                          |LED_SD_Pin|LED_CAN_Pin|LED_TELEMETRY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = SDIO_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(SDIO_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PDPin PDPin PDPin PDPin
                           PDPin PDPin PDPin PDPin */
  GPIO_InitStruct.Pin = DIN0_Pin|DIN1_Pin|DIN2_Pin|DIN3_Pin
                          |DIN4_Pin|DIN5_Pin|DIN6_Pin|DIN7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = ESP_COMM_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ESP_COMM_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
