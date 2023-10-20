/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Monolith TMA-1 datalogger STM32 firmware.
  *
  * Oh Byung-Jun (mail@luftaquila.io)
  * Ajou University, Korea.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "rtc.h"
#include "sdio.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "logger.h"
#include "types.h"
#include "ringbuffer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern int SYS_LOG(LOG_LEVEL level, LOG_SOURCE source, int key);

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern uint32_t to_uint(uint8_t *str, char garbage);
extern uint32_t drop_point(uint8_t *str);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// system log
FIL logfile;
LOG syslog;

// system state
SYSTEM_STATE sys_state;

// timer alarm flag
uint32_t timer_flag = 0;

// rtc buffer
uint8_t rtc[25]; // YYYY-MM-DD-HH-MM-SS

// SD card write buffer
ring_buffer_t SD_BUFFER;
uint8_t SD_BUFFER_ARR[1 << 12]; // 4KB

// serial log output buffer
#ifdef ENABLE_SERIAL
uint32_t serial_flag = 0;
ring_buffer_t SERIAL_BUFFER;
uint8_t SERIAL_BUFFER_ARR[1 << 14]; // 16KB
#endif

// telemetry transmission flag and buffer
#ifdef ENABLE_TELEMETRY
uint32_t telemetry_flag = 0;
uint32_t handshake_flag = 0;
ring_buffer_t TELEMETRY_BUFFER;
uint8_t TELEMETRY_BUFFER_ARR[1 << 14]; // 16KB
#endif

// CAN RX header and buffer
#ifdef ENABLE_MONITOR_CAN
CAN_RxHeaderTypeDef can_rx_header;
uint8_t can_rx_data[8];
#endif

// adc conversion buffers
uint32_t adc_flag = 0;
uint32_t adc_sys_value[2] = { 0, };
#ifdef ENABLE_MONITOR_ANALOG
uint32_t adc_ain_value[ADC_COUNT] = { 0, };
#endif

// timer pulse input capture flag and data
#ifdef ENABLE_MONITOR_PULSE
uint32_t pulse_flag = 0;
uint32_t pulse_value[PULSE_CH_COUNT] = { 0, }; // microsecond
uint32_t pulse_buffer_0[PULSE_CH_COUNT] = { 0, };
uint32_t pulse_buffer_1[PULSE_CH_COUNT] = { 0, };
#endif

// accelerometer receive buffer
#ifdef ENABLE_MONITOR_ACCELEROMETER
uint32_t accelerometer_flag = false;
uint8_t accelerometer_value[6];
#endif

// GPS receive flag and buffer
#ifdef ENABLE_MONITOR_GPS
uint32_t gps_flag = false;
uint8_t gps_data[1 << 7]; // 128B
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
__attribute__((weak)) void _close(void){}
__attribute__((weak)) void _lseek(void){}
__attribute__((weak)) void _read(void){}

void GPS_PARSE(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, uint8_t *ptr, int len) {
  HAL_UART_Transmit(UART_DEBUG, (uint8_t *)ptr, (uint16_t)len, 30);
  return (len);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_RTC_Init();
  MX_TIM1_Init();
  MX_TIM5_Init();
  MX_CAN1_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_SDIO_SD_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_FATFS_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */

  /********** CORE SYSTEM STARTUP **********/
  int ret;
  DEBUG_MSG("[%8lu] [INF] core system is in startup\r\n", HAL_GetTick());


  /********** RTC boot time check **********/
  DATETIME boot;
  RTC_READ(&boot);
  DEBUG_MSG("[%8lu] [INF] RTC time is 20%02d-%02d-%02d %02d:%02d:%02d\r\n", HAL_GetTick(), boot.year, boot.month, boot.date, boot.hour, boot.minute, boot.second);


  /********** SD card initialization **********/
  ret = SD_SETUP(&boot);

  if (ret == SYS_OK) {
    sys_state.SD = true;
    HAL_GPIO_WritePin(GPIOE, LED_SD_Pin, GPIO_PIN_SET);

    SYS_LOG(LOG_INFO, SYS, SYS_SD_INIT);

    DEBUG_MSG("[%8lu] [ OK] SD card setup\r\n", HAL_GetTick());
  } else {
    sys_state.SD = false;
    HAL_GPIO_WritePin(GPIOE, LED_SD_Pin, GPIO_PIN_RESET);

    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, SYS_SD_INIT);

    DEBUG_MSG("[%8lu] [ERR] SD card setup failed: %d\r\n", HAL_GetTick(), ret);
  }


  /********** core system initialization **********/
  HAL_GPIO_WritePin(GPIOA, LED_ONBOARD_0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LED_ONBOARD_1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, LED_CUSTOM_0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, LED_CUSTOM_1_Pin, GPIO_PIN_RESET);

  sys_state.ERR = false;
  sys_state.CAN = false;
  sys_state.TELEMETRY = false;

  HAL_GPIO_WritePin(GPIOE, LED_ERR_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, LED_HEARTBEAT_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_RESET);

  // RTC time sync by UART
  HAL_UART_Receive_IT(UART_DEBUG, rtc, 20);

  SYS_LOG(LOG_INFO, SYS, SYS_CORE_INIT);

  DEBUG_MSG("[%8lu] [ OK] core system setup\r\n", HAL_GetTick());


  /********** UART log output port initialization **********/
#ifdef ENABLE_SERIAL
  ret = SERIAL_SETUP();

  if (ret == SYS_OK) {
    SYS_LOG(LOG_INFO, SYS, SYS_SERIAL_INIT);

    DEBUG_MSG("[%8lu] [ OK] serial log output port setup\r\n", HAL_GetTick());
  } else {
    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, SYS_SERIAL_INIT);

    DEBUG_MSG("[%8lu] [ERR] serial log output port setup failed: %d\r\n", HAL_GetTick(), ret);
  }
#endif


  /********** ESP32 telemetry initialization **********/
#ifdef ENABLE_TELEMETRY
  ret = TELEMETRY_SETUP();

  if (ret == SYS_OK) {
    sys_state.TELEMETRY = true;

    // flash 3 times
    HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_RESET);

    SYS_LOG(LOG_INFO, SYS, SYS_TELEMETRY_INIT);

    DEBUG_MSG("[%8lu] [ OK] TELEMETRY setup\r\n", HAL_GetTick());
  } else {
    sys_state.TELEMETRY = false;
    HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_RESET);

    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, SYS_TELEMETRY_INIT);

    DEBUG_MSG("[%8lu] [ERR] TELEMETRY setup failed: %d\r\n", HAL_GetTick(), ret);
  }
#endif


  /********** CAN transceiver initialization **********/
#ifdef ENABLE_MONITOR_CAN
  ret = CAN_SETUP();

  if (ret == SYS_OK) {
    sys_state.CAN = true;
    HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_SET);

    SYS_LOG(LOG_INFO, SYS, CAN_INIT);

    DEBUG_MSG("[%8lu] [ OK] CAN transceiver setup\r\n", HAL_GetTick());
  } else {
    sys_state.CAN = false;
    HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_RESET);

    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, CAN_INIT);

    DEBUG_MSG("[%8lu] [ERR] CAN transceiver setup failed: %d\r\n", HAL_GetTick(), ret);
  }
#endif


  /********** digital input initialization **********/
#ifdef ENABLE_MONITOR_DIGITAL
  ret = DIGITAL_SETUP();

  if (ret == SYS_OK) {
    SYS_LOG(LOG_INFO, SYS, DIGITAL_INIT);

    DEBUG_MSG("[%8lu] [ OK] digital input setup\r\n", HAL_GetTick());
  } else {
    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, DIGITAL_INIT);

    DEBUG_MSG("[%8lu] [ERR] digital input setup failed: %d\r\n", HAL_GetTick(), ret);
  }
#endif


  /********** analog input initialization **********/
#ifdef ENABLE_MONITOR_ANALOG
  ret = ANALOG_SETUP();

  if (ret == SYS_OK) {
    SYS_LOG(LOG_INFO, SYS, ANALOG_INIT);

    DEBUG_MSG("[%8lu] [ OK] analog input setup\r\n", HAL_GetTick());
  } else {
    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, ANALOG_INIT);

    DEBUG_MSG("[%8lu] [ERR] analog input setup failed: %d\r\n", HAL_GetTick(), ret);
  }
#endif


  /********** pulse input initialization **********/
#ifdef ENABLE_MONITOR_PULSE
  ret = PULSE_SETUP();

  if (ret == SYS_OK) {
    SYS_LOG(LOG_INFO, SYS, PULSE_INIT);

    DEBUG_MSG("[%8lu] [ OK] pulse input setup\r\n", HAL_GetTick());
  } else {
    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, PULSE_INIT);

    DEBUG_MSG("[%8lu] [ERR] pulse input setup failed: %d\r\n", HAL_GetTick(), ret);
  }
#endif


  /********** ADXL345 accelerometer initialization **********/
#ifdef ENABLE_MONITOR_ACCELEROMETER
  ret = ACCELEROMETER_SETUP();

  if (ret == SYS_OK) {
    SYS_LOG(LOG_INFO, SYS, ACCELEROMETER_INIT);

    DEBUG_MSG("[%8lu] [ OK] accelerometer setup\r\n", HAL_GetTick());
  } else {
    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, ACCELEROMETER_INIT);

    DEBUG_MSG("[%8lu] [ERR] accelerometer setup failed: %d\r\n", HAL_GetTick(), ret);
  }
#endif


  /********** NEO-7M GPS initialization **********/
#ifdef ENABLE_MONITOR_GPS
  ret = GPS_SETUP();

  if (ret == SYS_OK) {
    SYS_LOG(LOG_INFO, SYS, GPS_INIT);

    DEBUG_MSG("[%8lu] [ OK] GPS setup\r\n", HAL_GetTick());
  } else {
    syslog.value[0] = (uint8_t)ret;
    SYS_LOG(LOG_ERROR, SYS, GPS_INIT);

    DEBUG_MSG("[%8lu] [ERR] GPS setup failed: %d\r\n", HAL_GetTick(), ret);
  }
#endif


  /********** CORE SYSTEM STARTUP COMPLETE **********/
  SYS_LOG(LOG_INFO, SYS, SYS_READY);
  DEBUG_MSG("[%8lu] [ OK] CORE SYSTEM STARTUP COMPLETE\r\n", HAL_GetTick());

  HAL_TIM_Base_Start_IT(&htim1); // start 100ms periodic timer

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* check flags */
    // system monitor: core temperature and input voltage
    if (adc_flag & (1 << FLAG_ADC_SYS)) {
      adc_flag &= ~(1 << FLAG_ADC_SYS);

      *(uint16_t *)(syslog.value + 0) = (uint16_t)adc_sys_value[0];
      *(uint16_t *)(syslog.value + 2) = (uint16_t)adc_sys_value[1];
      SYS_LOG(LOG_INFO, ANALOG, ANALOG_SYS);
    }

#ifdef ENABLE_MONITOR_ANALOG
    if (adc_flag & (1 << FLAG_ADC_AIN)) {
      adc_flag &= ~(1 << FLAG_ADC_AIN);

      *(uint16_t *)(syslog.value + 0) = (uint16_t)adc_ain_value[0];
      *(uint16_t *)(syslog.value + 2) = (uint16_t)adc_ain_value[1];
      *(uint16_t *)(syslog.value + 4) = (uint16_t)adc_ain_value[2];
      *(uint16_t *)(syslog.value + 6) = (uint16_t)adc_ain_value[3];
      SYS_LOG(LOG_INFO, ANALOG, ANALOG_DATA);
    }
#endif

#ifdef ENABLE_MONITOR_PULSE
    if (pulse_flag & (1 << PULSE_SET)) {
      // x10 us
      *(uint16_t *)(syslog.value + 0) = (uint16_t)(pulse_value[PULSE_CH0] / 10);
      *(uint16_t *)(syslog.value + 2) = (uint16_t)(pulse_value[PULSE_CH1] / 10);
      *(uint16_t *)(syslog.value + 4) = (uint16_t)(pulse_value[PULSE_CH2] / 10);
      *(uint16_t *)(syslog.value + 6) = (uint16_t)(pulse_value[PULSE_CH3] / 10);
      SYS_LOG(LOG_INFO, PULSE, PULSE_DATA);

      pulse_flag = 0;
    }
#endif

#ifdef ENABLE_MONITOR_ACCELEROMETER
    if (accelerometer_flag == true) {
      accelerometer_flag = false;

      *(uint64_t *)syslog.value = *(uint64_t *)accelerometer_value;
      SYS_LOG(LOG_INFO, ACCELEROMETER, ACCELEROMETER_DATA);
    }
#endif

#ifdef ENABLE_MONITOR_GPS
    if (gps_flag == true) {
      gps_flag = false;
      GPS_PARSE();
    }
#endif

    /* handle recorded LOGs */
    SD_WRITE();

#ifdef ENABLE_SERIAL
    SERIAL_TRANSMIT_LOG();
#endif

#ifdef ENABLE_TELEMETRY
    TELEMETRY_TRANSMIT_LOG();
#endif

    /* check timer flags */
    if (timer_flag & (1 << FLAG_TIMER_100ms)) {
      timer_flag &= ~(1 << FLAG_TIMER_100ms);
      TIMER_100ms();
    }

    if (timer_flag & (1 << FLAG_TIMER_1s)) {
      timer_flag &= ~(1 << FLAG_TIMER_1s);
      TIMER_1s();
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/* USER CODE BEGIN 4 */
void TIMER_100ms(void) {
#ifdef ENABLE_MONITOR_DIGITAL
  /* record digital input channels */
  syslog.value[0] = !HAL_GPIO_ReadPin(GPIOD, DIN0_Pin);
  syslog.value[1] = !HAL_GPIO_ReadPin(GPIOD, DIN1_Pin);
  syslog.value[2] = !HAL_GPIO_ReadPin(GPIOD, DIN2_Pin);
  syslog.value[3] = !HAL_GPIO_ReadPin(GPIOD, DIN3_Pin);
  syslog.value[4] = !HAL_GPIO_ReadPin(GPIOD, DIN4_Pin);
  syslog.value[5] = !HAL_GPIO_ReadPin(GPIOD, DIN5_Pin);
  syslog.value[6] = !HAL_GPIO_ReadPin(GPIOD, DIN6_Pin);
  syslog.value[7] = !HAL_GPIO_ReadPin(GPIOD, DIN7_Pin);
  SYS_LOG(LOG_INFO, DIGITAL, DIGITAL_DATA);
#endif

#ifdef ENABLE_MONITOR_ANALOG
  /* start analog input channels ADC conversion */
  if (!(adc_flag & (1 << FLAG_ADC_AIN))) {
    HAL_ADC_Start_DMA(&hadc2, adc_ain_value, 4);
  }
#endif

#ifdef ENABLE_MONITOR_PULSE
  PULSE_CAPTURE();
#endif

#ifdef ENABLE_MONITOR_ACCELEROMETER
  /* this takes 30ms EVERY TIME if the module is NOT CONNECTED */
  if (accelerometer_flag == false) {
    HAL_I2C_Mem_Read_IT(I2C_ACCELEROMETER, ACC_I2C_ADDR, 0x32, 1, accelerometer_value, 6);
  }
#endif
}

void TIMER_1s(void) {
  /* internal temperature and input voltage monitor */
  if (!(adc_flag & (1 << FLAG_ADC_SYS))) {
    HAL_ADC_Start_DMA(&hadc1, adc_sys_value, 2);
  }

  /* SD card sync*/
  SD_SYNC();

  /* heartbeat LED */
  static bool heartbeat = true;
  if (heartbeat) {
    HAL_GPIO_WritePin(GPIOE, LED_HEARTBEAT_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(GPIOE, LED_HEARTBEAT_Pin, GPIO_PIN_RESET);
  }
  heartbeat = !heartbeat;
}

#ifdef ENABLE_MONITOR_GPS
void GPS_PARSE(void) {
  static NMEA_GPRMC gprmc;
  static GPS_COORD gps_coord;
  static GPS_VECTOR gps_vector;
  static GPS_DATETIME gps_datetime;

  if (!strncmp((char *)gps_data, "$GPRMC", 6)) {

    // parse NMEA GPRMC sentence
    gprmc.id = gps_data;
    gprmc.utc_time = FIND_AND_NUL(gprmc.id, gprmc.utc_time, ',');
    gprmc.status = FIND_AND_NUL(gprmc.utc_time, gprmc.status, ',');

    // proceed only if GPS fix is valid
    if (*gprmc.status == 'A') {
      gprmc.lat = FIND_AND_NUL(gprmc.status, gprmc.lat, ',');
      gprmc.north = FIND_AND_NUL(gprmc.lat, gprmc.north, ',');
      gprmc.lon = FIND_AND_NUL(gprmc.north, gprmc.lon, ',');
      gprmc.east = FIND_AND_NUL(gprmc.lon, gprmc.east, ',');
      gprmc.speed = FIND_AND_NUL(gprmc.east, gprmc.speed, ',');
      gprmc.course = FIND_AND_NUL(gprmc.speed, gprmc.course, ',');
      gprmc.utc_date = FIND_AND_NUL(gprmc.course, gprmc.utc_date, ',');
      gprmc.others = FIND_AND_NUL(gprmc.utc_date, gprmc.others, ',');

      // process GPS coordinates
      gps_coord.lat = to_uint(gprmc.lat, '.');
      gps_coord.lon = to_uint(gprmc.lon, '.');

      // process GPS speed and course
      gps_vector.speed = (int)(atof((char *)gprmc.speed) * 100);
      gps_vector.course = drop_point(gprmc.course);

      // process GPS datetime
      gps_datetime.utc_date = atoi((char *)gprmc.utc_date);
      gps_datetime.utc_time = drop_point(gprmc.utc_time);

      *(uint64_t *)syslog.value = *(uint64_t *)&gps_coord;
      SYS_LOG(LOG_INFO, GPS, GPS_POS);

      *(uint64_t *)syslog.value = *(uint64_t *)&gps_vector;
      SYS_LOG(LOG_INFO, GPS, GPS_VEC);

      *(uint64_t *)syslog.value = *(uint64_t *)&gps_datetime;
      SYS_LOG(LOG_INFO, GPS, GPS_TIME);
    }
  }
}
#endif
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {

  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
