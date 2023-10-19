/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
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
#include "can.h"

/* USER CODE BEGIN 0 */
extern LOG syslog;
extern SYSTEM_STATE sys_state;

#ifdef ENABLE_MONITOR_CAN
extern CAN_RxHeaderTypeDef can_rx_header;
extern uint8_t can_rx_data[8];

/* CAN message received */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  int ret = HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &can_rx_header, can_rx_data);

  if (ret != HAL_OK) {
    sys_state.CAN = false;
    HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_RESET);

    DEBUG_MSG("[%8lu] [ERR] CAN RX failed: %d\r\n", HAL_GetTick(), ret);

    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, CAN_ERR_RXMSGFAIL);
  }

  if (sys_state.CAN != true) {
    sys_state.CAN = true;
    HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_SET);
  }

  *(uint64_t *)syslog.value = *(uint64_t *)can_rx_data;
  SYS_LOG(LOG_INFO, CAN, can_rx_header.StdId);
}

/* CAN error occured */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
  sys_state.CAN = false;
  HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_RESET);

  DEBUG_MSG("[%8lu] [ERR] CAN ERROR occured\r\n", HAL_GetTick());

  *(uint32_t *)syslog.value = HAL_CAN_GetError(hcan);
  SYS_LOG(LOG_ERROR, SYS, CAN_ERR_CANERR);

  HAL_CAN_ResetError(hcan);
}

/* CAN RX FIFO full */
void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan) {
  sys_state.CAN = false;
  HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_RESET);

  DEBUG_MSG("[%8lu] [ERR] CAN RX FIFO full\r\n", HAL_GetTick());

  *(uint32_t *)syslog.value = HAL_CAN_GetState(hcan);
  SYS_LOG(LOG_ERROR, SYS, CAN_ERR_FIFOFULL);

  HAL_CAN_ResetError(hcan);
}

int CAN_SETUP(void) {
  // receive all CAN ID
  CAN_FilterTypeDef CAN_FILTER;
  CAN_FILTER.FilterBank = 0;
  CAN_FILTER.FilterMode = CAN_FILTERMODE_IDMASK;
  CAN_FILTER.FilterScale = CAN_FILTERSCALE_32BIT;
  CAN_FILTER.FilterIdHigh = 0x0;
  CAN_FILTER.FilterMaskIdHigh = 0x0;
  CAN_FILTER.FilterIdLow = 0x0;
  CAN_FILTER.FilterMaskIdLow = 0x0;
  CAN_FILTER.FilterFIFOAssignment = CAN_RX_FIFO0;
  CAN_FILTER.FilterActivation = ENABLE;

  int ret = HAL_CAN_ConfigFilter(&hcan1, &CAN_FILTER);
  if (ret != HAL_OK) {
    DEBUG_MSG("[%8lu] [ERR] CAN filter config failed: %d\r\n", HAL_GetTick(), ret);
    return 1;
  }

  ret = HAL_CAN_Start(&hcan1);
  if (ret != HAL_OK) {
    DEBUG_MSG("[%8lu] [ERR] CAN start failed: %d\r\n", HAL_GetTick(), ret);
    return 2;
  }

  ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
  if (ret != HAL_OK) {
    DEBUG_MSG("[%8lu] [ERR] CAN RX_MSG_PENDING activation failed: %d\r\n", HAL_GetTick(), ret);
    return 3;
  }

  ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_FULL);
  if (ret != HAL_OK) {
    DEBUG_MSG("[%8lu] [ERR] CAN RX_FULL activation failed: %d\r\n", HAL_GetTick(), ret);
    return 4;
  }

  ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_OVERRUN);
  if (ret != HAL_OK) {
    DEBUG_MSG("[%8lu] [ERR] CAN RX_OVERRUN activation failed: %d\r\n", HAL_GetTick(), ret);
    return 5;
  }

  ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_BUSOFF);
  if (ret != HAL_OK) {
    DEBUG_MSG("[%8lu] [ERR] CAN BUSOFF activation failed: %d\r\n", HAL_GetTick(), ret);
    return 6;
  }

  ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_ERROR);
  if (ret != HAL_OK) {
    DEBUG_MSG("[%8lu] [ERR] CAN ERROR activation failed: %d\r\n", HAL_GetTick(), ret);
    return 7;
  }

  return SYS_OK;
}
#endif

/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;

/* CAN1 init function */
void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = CAN_PRESCALER;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_TIMESEG1;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = ENABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PD0     ------> CAN1_RX
    PD1     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN1 GPIO Configuration
    PD0     ------> CAN1_RX
    PD1     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_0|GPIO_PIN_1);

    /* CAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
