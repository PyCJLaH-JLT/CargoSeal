#ifndef CS_FAKE_MAIN_H
#define CS_FAKE_MAIN_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { unsigned unused; } I2C_HandleTypeDef;
typedef struct { unsigned unused; } UART_HandleTypeDef;
typedef struct { uint32_t ClockPrescaler,Resolution,DataAlign,ScanConvMode,EOCSelection,NbrOfConversion,
ExternalTrigConv,ExternalTrigConvEdge,Overrun,SamplingTimeCommon1,SamplingTimeCommon2,TriggerFrequencyMode; } ADC_InitTypeDef;
typedef struct { void *Instance;ADC_InitTypeDef Init; } ADC_HandleTypeDef;
typedef struct { uint32_t Pin,Mode,Pull; } GPIO_InitTypeDef;
typedef struct { uint32_t Channel,Rank,SamplingTime; } ADC_ChannelConfTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
#define GPIOA ((void *)1)
#define ADC1 ((void *)2)
#define Led_GPIO_Port GPIOA
#define Led_Pin GPIO_PIN_5
#define GPIO_PIN_0 1
#define GPIO_PIN_1 2
#define GPIO_PIN_4 16
#define GPIO_PIN_5 32
#define GPIO_MODE_ANALOG 0
#define GPIO_MODE_INPUT 1
#define GPIO_MODE_OUTPUT_PP 2
#define GPIO_NOPULL 0
#define GPIO_PULLDOWN 1
#define USART2_IRQn 1
#define ADC_CLOCK_SYNC_PCLK_DIV4 4
#define ADC_RESOLUTION_12B 12
#define ADC_DATAALIGN_RIGHT 0
#define ADC_SCAN_DISABLE 0
#define ADC_EOC_SINGLE_CONV 0
#define ADC_SOFTWARE_START 0
#define ADC_EXTERNALTRIGCONVEDGE_NONE 0
#define ADC_OVR_DATA_PRESERVED 0
#define ADC_SAMPLETIME_160CYCLES_5 160
#define ADC_TRIGGER_FREQ_LOW 0
#define ADC_CHANNEL_0 0
#define ADC_REGULAR_RANK_1 1
#define ADC_SAMPLINGTIME_COMMON_1 1
#define I2C_MEMADD_SIZE_8BIT 1
#define HAL_I2C_ERROR_AF 1
#define __HAL_RCC_ADC_CLK_ENABLE() ((void)0)
#define __HAL_UART_CLEAR_OREFLAG(x) ((void)(x))
uint32_t HAL_GetTick(void);
uint32_t HAL_I2C_GetError(I2C_HandleTypeDef *);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *,uint16_t,uint16_t,uint16_t,uint8_t *,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *,uint16_t,uint16_t,uint16_t,uint8_t *,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *,uint8_t *,uint16_t);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *,uint8_t *,uint16_t);
void HAL_GPIO_Init(void *,GPIO_InitTypeDef *);
void HAL_GPIO_WritePin(void *,uint16_t,GPIO_PinState);
GPIO_PinState HAL_GPIO_ReadPin(void *,uint16_t);
void HAL_NVIC_SetPriority(unsigned,unsigned,unsigned);
void HAL_NVIC_EnableIRQ(unsigned);
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *,ADC_ChannelConfTypeDef *);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *,uint32_t);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *);
#endif

