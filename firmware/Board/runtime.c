#include "runtime.h"
#include "imu_bus.h"
#include "main.h"
#include "../Policy/container.h"
#include "../Policy/commands.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
static ADC_HandleTypeDef adc;
static cs_container container;
static cs_light light;
static cs_imu_sample imu;
static cs_mpu6050 mpu;
static cs_bus_result imu_result;
static cs_line_reader reader;
static uint8_t raw[14], rx_byte, tx_block[128];
static char tx_ring[4096];
static uint8_t rx_ring[512];
static unsigned tx_head, tx_tail;
static volatile unsigned rx_head, rx_tail;
static volatile bool tx_busy;
static uint32_t tx_dropped, next_imu, next_adc, last_retry, last_tick;
static volatile uint32_t rx_dropped;
static bool baseline_logged;
static void text(const char *format, ...)
{
    char line[256];
    va_list args; va_start(args,format);
    int n=vsnprintf(line,sizeof line,format,args); va_end(args);
    if(n<0 || (size_t)n>=sizeof line || (unsigned)n > (tx_tail+sizeof tx_ring-tx_head-1U)%sizeof tx_ring) {
        ++tx_dropped; return;
    }
    for(int i=0;i<n;++i){tx_ring[tx_head]=line[i];tx_head=(tx_head+1U)%sizeof tx_ring;}
}
static void transmit(void)
{
    if(tx_busy || tx_head==tx_tail) return;
    unsigned count=0, tail=tx_tail;
    while(tail!=tx_head && count<sizeof tx_block) {
        tx_block[count++]=(uint8_t)tx_ring[tail]; tail=(tail+1U)%sizeof tx_ring;
    }
    tx_busy=true;
    if(HAL_UART_Transmit_IT(&huart2,tx_block,(uint16_t)count)==HAL_OK) tx_tail=tail;
    else tx_busy=false;
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{ if(uart==&huart2) tx_busy=false; }
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    if(uart!=&huart2) return;
    unsigned head=(rx_head+1U)%sizeof rx_ring;
    if(head==rx_tail) ++rx_dropped;
    else {rx_ring[rx_head]=rx_byte;rx_head=head;}
    (void)HAL_UART_Receive_IT(&huart2,&rx_byte,1);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if(uart!=&huart2) return;
    ++rx_dropped;
    __HAL_UART_CLEAR_OREFLAG(uart);
    (void)HAL_UART_Receive_IT(uart,&rx_byte,1);
}
static void event(void *ctx,const cs_event *e)
{
    (void)ctx;
    const char *source="IMU", *unit="mg";
    if(e->kind==CS_NEARBY){source="PIR";unit="bool";}
    if(e->kind==CS_LIGHT_CHANGE){source="LDR";unit="ADC_code";}
    if(e->kind==CS_SENSOR_FAULT || e->kind==CS_SENSOR_RECOVERED){source="I2C";unit="result_code";}
    if(e->kind==CS_MODE_CHANGE){
        text("EVENT seq=%lu time_ms=%lu mode=%s event=MODE_CHANGE source=UART previous=%s value=%s unit=mode valid=1 alarm=%u\r\n",
             (unsigned long)e->seq,(unsigned long)e->time_ms,cs_mode_name(e->mode),
             cs_mode_name((cs_mode)e->value),cs_mode_name((cs_mode)e->secondary),container.alarm);
        return;
    }
    text("EVENT seq=%lu time_ms=%lu mode=%s event=%s source=%s value=%ld unit=%s secondary=%ld secondary_unit=%s valid=%u alarm=%u\r\n",
         (unsigned long)e->seq,(unsigned long)e->time_ms,cs_mode_name(e->mode),cs_event_name(e->kind),source,
         (long)e->value,unit,(long)e->secondary,e->kind==CS_LIGHT_CHANGE?"ADC_code":
         (e->kind==CS_MOVE||e->kind==CS_DISTURBANCE||e->kind==CS_IMPACT?"mdps":"none"),e->valid,container.alarm);
}
static bool adc_init(void)
{
    __HAL_RCC_ADC_CLK_ENABLE();
    GPIO_InitTypeDef pin={0}; pin.Pin=GPIO_PIN_0;pin.Mode=GPIO_MODE_ANALOG;pin.Pull=GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA,&pin);
    adc.Instance=ADC1;
    adc.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4;
    adc.Init.Resolution=ADC_RESOLUTION_12B;
    adc.Init.DataAlign=ADC_DATAALIGN_RIGHT;
    adc.Init.ScanConvMode=ADC_SCAN_DISABLE;
    adc.Init.EOCSelection=ADC_EOC_SINGLE_CONV;
    adc.Init.NbrOfConversion=1;
    adc.Init.ExternalTrigConv=ADC_SOFTWARE_START;
    adc.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.Overrun=ADC_OVR_DATA_PRESERVED;
    adc.Init.SamplingTimeCommon1=ADC_SAMPLETIME_160CYCLES_5;
    adc.Init.SamplingTimeCommon2=ADC_SAMPLETIME_160CYCLES_5;
    adc.Init.TriggerFrequencyMode=ADC_TRIGGER_FREQ_LOW;
    if(HAL_ADC_Init(&adc)!=HAL_OK) return false;
    ADC_ChannelConfTypeDef channel={0};
    channel.Channel=ADC_CHANNEL_0;channel.Rank=ADC_REGULAR_RANK_1;
    channel.SamplingTime=ADC_SAMPLINGTIME_COMMON_1;
    if(HAL_ADC_ConfigChannel(&adc,&channel)!=HAL_OK) return false;
    return HAL_ADCEx_Calibration_Start(&adc)==HAL_OK;
}
static void adc_read(uint32_t now)
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
    bool valid=HAL_ADC_Start(&adc)==HAL_OK && HAL_ADC_PollForConversion(&adc,2)==HAL_OK;
    uint16_t value=valid?(uint16_t)HAL_ADC_GetValue(&adc):light.raw;
    (void)HAL_ADC_Stop(&adc);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
    cs_light_measure(&light,value,valid);
    cs_container_light(&container,&light,now);
    if(light.ready && !baseline_logged){
        baseline_logged=true;
        text("CALIBRATION time_ms=%lu samples=20 E0=%u unit=ADC_code ready=1\r\n",(unsigned long)now,light.baseline);
    }
}
static void status(uint32_t now)
{
    text("STATUS mode=%s alarm=%u imu_valid=%u adc_raw=%u adc_valid=%u light_ready=%u E0=%u E=%u pir=%u uptime_ms=%lu tx_dropped=%lu rx_dropped=%lu\r\n",
         cs_mode_name(container.mode),container.alarm,container.imu_valid,light.raw,light.valid,light.ready,
         light.baseline,light.average,container.pir.high,(unsigned long)now,(unsigned long)tx_dropped,(unsigned long)rx_dropped);
}
static void selftest(void)
{
    text("SELFTEST device=MPU6050 valid=%u result=%u address=0x%02X accel_range_g=8 gyro_range_dps=250\r\n",container.imu_valid,imu_result,mpu.address);
    text("SELFTEST device=PIR gpio_readable=1 high=%u\r\n",container.pir.high);
    text("SELFTEST device=LDR adc_valid=%u raw=%u baseline_ready=%u\r\n",light.valid,light.raw,light.ready);
}
static void read_imu(uint32_t now)
{
    uint8_t id=0;cs_imu_sample sample;uint8_t bytes[14];
    cs_bus_result r=cs_board_imu_read(NULL,mpu.address,0x75,&id,1);
    if(r==CS_BUS_OK) r=cs_mpu_read(&mpu,&sample,bytes);
    if(r!=CS_BUS_OK){text("READ_IMU time_ms=%lu valid=0 result=%u\r\n",(unsigned long)now,r);return;}
    char hex[43];
    for(unsigned i=0;i<14;++i) snprintf(hex+3U*i,4,"%02X ",bytes[i]);
    text("READ_IMU time_ms=%lu valid=1 id=0x%02X raw=%sax_mg=%ld ay_mg=%ld az_mg=%ld gx_mdps=%ld gy_mdps=%ld gz_mdps=%ld\r\n",
         (unsigned long)now,id,hex,(long)(sample.acceleration_g[0]*1000),(long)(sample.acceleration_g[1]*1000),
         (long)(sample.acceleration_g[2]*1000),(long)(sample.rotation_dps[0]*1000),
         (long)(sample.rotation_dps[1]*1000),(long)(sample.rotation_dps[2]*1000));
}
static void command(cs_command cmd,uint32_t now)
{
    switch(cmd){
    case CS_CMD_STATUS: status(now);break;
    case CS_CMD_ACK: text(cs_container_ack(&container)?"OK ACK\r\n":"ERR ACTIVE\r\n");break;
    case CS_CMD_TRANSPORT: case CS_CMD_INSTALLED:
        cs_container_mode(&container,cmd==CS_CMD_TRANSPORT?CS_TRANSPORT:CS_INSTALLED,now);
        text("OK MODE %s\r\n",cs_mode_name(container.mode));break;
    case CS_CMD_SELFTEST:selftest();break;
    case CS_CMD_READ_IMU:read_imu(now);break;
    case CS_CMD_ADDRESS_68:case CS_CMD_ADDRESS_69:
        mpu.address=cmd==CS_CMD_ADDRESS_68?0x68:0x69;
        imu_result=CS_BUS_NACK;last_retry=now-1000U;
        text("OK IMU ADDRESS 0x%02X\r\n",mpu.address);break;
    default:text("ERR COMMAND\r\n");break;
    }
}
void cs_runtime_start(void)
{
    cs_container_init(&container,event,NULL);
    GPIO_InitTypeDef pin={0};
    pin.Pin=GPIO_PIN_1;pin.Mode=GPIO_MODE_INPUT;pin.Pull=GPIO_PULLDOWN;HAL_GPIO_Init(GPIOA,&pin);
    pin.Pin=GPIO_PIN_4;pin.Mode=GPIO_MODE_OUTPUT_PP;pin.Pull=GPIO_NOPULL;HAL_GPIO_Init(GPIOA,&pin);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
    HAL_NVIC_SetPriority(USART2_IRQn,1,0);HAL_NVIC_EnableIRQ(USART2_IRQn);
    (void)HAL_UART_Receive_IT(&huart2,&rx_byte,1);
    mpu=(cs_mpu6050){{NULL,cs_board_imu_read,cs_board_imu_write},0x68};
    text("STM32C031C6 ready\r\nBOOT CargoSeal mode=INSTALLED imu_period_ms=20 adc_period_ms=50\r\n");
    if(!adc_init()) text("SELFTEST device=ADC init=ERROR\r\n");
    uint32_t now=HAL_GetTick();
    imu_result=cs_mpu_start(&mpu);last_retry=now;
    if(imu_result==CS_BUS_OK) imu_result=cs_mpu_read(&mpu,&imu,raw);
    cs_container_imu(&container,&imu,imu_result,HAL_GetTick());
    cs_container_pir(&container,HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1)==GPIO_PIN_SET,HAL_GetTick());
    adc_read(HAL_GetTick());selftest();
    next_imu=HAL_GetTick();next_adc=next_imu;last_tick=next_imu;
}
void cs_runtime_poll(void)
{
    transmit();
    uint32_t now=HAL_GetTick();
    if(cs_elapsed(now,next_imu,20)){
        next_imu+=20;if(cs_elapsed(now,next_imu,20))next_imu=now;
        cs_container_pir(&container,HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1)==GPIO_PIN_SET,now);
        if(imu_result!=CS_BUS_OK && cs_elapsed(now,last_retry,1000)){
            last_retry=now;imu_result=cs_mpu_start(&mpu);
        }
        if(imu_result==CS_BUS_OK)imu_result=cs_mpu_read(&mpu,&imu,raw);
        cs_container_imu(&container,&imu,imu_result,HAL_GetTick());
    }
    now=HAL_GetTick();
    if(cs_elapsed(now,next_adc,50)){
        next_adc+=50;if(cs_elapsed(now,next_adc,50))next_adc=now;
        adc_read(now);
    }
    unsigned processed=0;
    while(rx_tail!=rx_head && processed++<64){
        uint8_t byte=rx_ring[rx_tail];rx_tail=(rx_tail+1U)%sizeof rx_ring;
        cs_command cmd;
        if(cs_line_byte(&reader,byte,&cmd)) command(cmd,HAL_GetTick());
    }
    now=HAL_GetTick();
    if(cs_elapsed(now,last_tick,1000)){
        last_tick+=1000;
        text("tick %lu\r\n",(unsigned long)(now/1000U));
    }
    bool led=container.alarm || ((now/1000U)&1U);
    HAL_GPIO_WritePin(Led_GPIO_Port,Led_Pin,led?GPIO_PIN_SET:GPIO_PIN_RESET);
}

