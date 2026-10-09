#include "fakes/main.h"
#include "../firmware/Board/runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *);
static uint32_t now, tx_due, imu_reads, adc_reads, adc_last, adc_max_gap;
static uint8_t registers[128], *rx_target, *pending;
static uint16_t pending_n, az=4096, adc_raw=4000;
static bool pir_high, led;
static char output[1000000];
static size_t output_n;
uint32_t HAL_GetTick(void){return now;}
uint32_t HAL_I2C_GetError(I2C_HandleTypeDef*x){(void)x;return HAL_I2C_ERROR_AF;}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef*h,uint16_t a,uint16_t r,uint16_t z,uint8_t*d,uint16_t n,uint32_t timeout)
{
 (void)h;(void)z;assert(timeout==10);
 if(a!=0xD0)return HAL_ERROR;
 if(r==0x3B){++imu_reads;memset(d,0,n);d[4]=(uint8_t)(az>>8);d[5]=(uint8_t)az;}
 else memcpy(d,registers+r,n);
 return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef*h,uint16_t a,uint16_t r,uint16_t z,uint8_t*d,uint16_t n,uint32_t timeout)
{(void)h;(void)z;assert(timeout==10);if(a!=0xD0)return HAL_ERROR;memcpy(registers+r,d,n);return HAL_OK;}
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef*h,uint8_t*d,uint16_t n)
{(void)h;assert(!pending);pending=d;pending_n=n;tx_due=now+(n*10000U+115199U)/115200U;return HAL_OK;}
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef*h,uint8_t*d,uint16_t n)
{(void)h;assert(n==1);rx_target=d;return HAL_OK;}
void HAL_GPIO_Init(void*p,GPIO_InitTypeDef*c){(void)p;(void)c;}
void HAL_GPIO_WritePin(void*p,uint16_t pin,GPIO_PinState v){(void)p;if(pin==GPIO_PIN_5)led=v==GPIO_PIN_SET;}
GPIO_PinState HAL_GPIO_ReadPin(void*p,uint16_t pin){(void)p;assert(pin==GPIO_PIN_1);return pir_high?GPIO_PIN_SET:GPIO_PIN_RESET;}
void HAL_NVIC_SetPriority(unsigned x,unsigned y,unsigned z){(void)x;(void)y;(void)z;}
void HAL_NVIC_EnableIRQ(unsigned x){(void)x;}
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef*h){assert(h->Init.Resolution==12);return HAL_OK;}
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef*h,ADC_ChannelConfTypeDef*c){(void)h;assert(c->Channel==0);return HAL_OK;}
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef*h){(void)h;return HAL_OK;}
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef*h)
{(void)h;++adc_reads;if(adc_reads>1 && now-adc_last>adc_max_gap)adc_max_gap=now-adc_last;adc_last=now;return HAL_OK;}
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef*h,uint32_t t){(void)h;assert(t==2);return HAL_OK;}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef*h){(void)h;return adc_raw;}
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef*h){(void)h;return HAL_OK;}
static void run(unsigned ms)
{
 for(unsigned i=0;i<ms;++i){
  ++now;
  if(pending && (int32_t)(now-tx_due)>=0){
   assert(output_n+pending_n<sizeof output);memcpy(output+output_n,pending,pending_n);output_n+=pending_n;
   output[output_n]=0;pending=NULL;HAL_UART_TxCpltCallback(&huart2);
  }
  cs_runtime_poll();
 }
}
static void inject(const char*s){for(;*s;++s){*rx_target=(uint8_t)*s;HAL_UART_RxCpltCallback(&huart2);}}
static unsigned count(const char*needle){unsigned n=0;const char*p=output;while((p=strstr(p,needle))){++n;p+=strlen(needle);}return n;}
int main(void)
{
 registers[0x75]=0x68;cs_runtime_start();run(1600);
 assert(count("STM32C031C6 ready")==1 && count("CALIBRATION")==1);
 assert(imu_reads>=80 && adc_reads==33 && adc_max_gap==50);
 inject("MODE TRANSPORT");run(100);assert(count("OK MODE")==0);
 inject("\nSTATUS\n");run(100);assert(strstr(output,"STATUS mode=TRANSPORT alarm=0 imu_valid=1"));
 az=5325;run(300);assert(count("event=MOVE")==1);
 az=4096;run(600);inject("MODE INSTALLED\n");run(100);
 az=10240;pir_high=true;adc_raw=3000;run(550);
 assert(count("event=IMPACT")==1 && count("event=NEARBY")==1 && count("event=LIGHT_CHANGE")==1 && led);
 inject("ACK\n");run(100);assert(count("ERR ACTIVE")==1);
 az=4096;adc_raw=4000;run(750);inject("ACK\n");run(100);assert(count("OK ACK")==1);
 inject("IMU ADDRESS 0x69\n");run(1200);assert(count("event=SENSOR_FAULT")==1);
 unsigned reads=adc_reads;pir_high=false;run(40);pir_high=true;run(100);assert(count("event=NEARBY")==2);
 assert(adc_reads>reads);
 inject("STATUS\nIMU ADDRESS 0x68\n");run(1200);assert(count("event=SENSOR_RECOVERED")==1);
 unsigned prior=count("STATUS mode=");
 for(unsigned i=0;i<20;++i){inject("STATUS\n");run(50);}
 run(400);
 assert(count("STATUS mode=")==prior+20 && strstr(output,"tx_dropped=0 rx_dropped=0"));
 assert(adc_max_gap==50);
 puts("PASS: runtime 20/50ms schedule, LF, events, ACK, address fault/recovery and 20 STATUS/s at modeled 115200 UART");
 return 0;
}

