#include "imu_bus.h"
#include "main.h"
/* Wokwi C0 lacks the TC transition needed by HAL Mem_Read. Use AUTOEND phases. */
static void byte_guard(void)
{
    uint32_t remaining=120U*(SystemCoreClock/1000000U);
    uint32_t previous=SysTick->VAL;
    while(remaining){
        uint32_t current=SysTick->VAL;
        uint32_t elapsed=previous>=current?previous-current:previous+SysTick->LOAD+1U-current;
        if(elapsed>=remaining)break;
        remaining-=elapsed;previous=current;
    }
}
static cs_bus_result wait_flag(uint32_t flag,uint32_t started)
{
    for(;;){
        uint32_t status=I2C1->ISR;
        if(status&I2C_ISR_NACKF)return CS_BUS_NACK;
        if(status&(I2C_ISR_BERR|I2C_ISR_ARLO|I2C_ISR_OVR))return CS_BUS_CONFIG;
        if(status&flag)return CS_BUS_OK;
        if((uint32_t)(HAL_GetTick()-started)>=10U)return CS_BUS_TIMEOUT;
    }
}
static void clear_transfer(void)
{
    if(I2C1->ISR&I2C_ISR_BUSY){
        I2C1->CR2|=I2C_CR2_STOP;
        I2C1->CR1&=~I2C_CR1_PE;
        I2C1->CR1|=I2C_CR1_PE;
    }
    I2C1->ICR=I2C_ICR_STOPCF|I2C_ICR_NACKCF|I2C_ICR_BERRCF|I2C_ICR_ARLOCF|I2C_ICR_OVRCF;
    I2C1->CR2=0;
}
static cs_bus_result send(uint8_t address,const uint8_t *bytes,size_t count,uint32_t start)
{
    I2C1->ICR=I2C_ICR_STOPCF|I2C_ICR_NACKCF;
    I2C1->CR2=((uint32_t)address<<1)|((uint32_t)count<<I2C_CR2_NBYTES_Pos)|I2C_CR2_AUTOEND|I2C_CR2_START;
    byte_guard();
    for(size_t i=0;i<count;++i){
        cs_bus_result r=wait_flag(I2C_ISR_TXIS,start);
        if(r!=CS_BUS_OK)return r;
        I2C1->TXDR=bytes[i];
        /* Model updates TXIS after a byte. 9 clocks at 100kHz + 30us margin. */
        byte_guard();
    }
    cs_bus_result result=wait_flag(I2C_ISR_STOPF,start);
    if(result==CS_BUS_OK)byte_guard();
    return result;
}
cs_bus_result cs_board_imu_read(void *ctx,uint8_t address,uint8_t reg,uint8_t *out,size_t count)
{
    (void)ctx;
    if(count==0 || count>255)return CS_BUS_CONFIG;
    uint32_t start=HAL_GetTick();
    clear_transfer();
    cs_bus_result result=send(address,&reg,1,start);
    if(result==CS_BUS_OK){
        I2C1->ICR=I2C_ICR_STOPCF|I2C_ICR_NACKCF;
        I2C1->CR2=((uint32_t)address<<1)|((uint32_t)count<<I2C_CR2_NBYTES_Pos)|
                  I2C_CR2_RD_WRN|I2C_CR2_AUTOEND|I2C_CR2_START;
        for(size_t i=0;i<count;++i){
            result=wait_flag(I2C_ISR_RXNE,start);
            if(result!=CS_BUS_OK)break;
            out[i]=(uint8_t)I2C1->RXDR;
        }
        if(result==CS_BUS_OK){
            result=wait_flag(I2C_ISR_STOPF,start);
            if(result==CS_BUS_OK)byte_guard();
        }
    }
    clear_transfer();
    return result;
}
cs_bus_result cs_board_imu_write(void *ctx,uint8_t address,uint8_t reg,const uint8_t *data,size_t count)
{
    (void)ctx;
    if(count!=1)return CS_BUS_CONFIG;
    uint8_t bytes[2]={reg,*data};uint32_t start=HAL_GetTick();
    clear_transfer();
    cs_bus_result result=send(address,bytes,2,start);
    clear_transfer();
    return result;
}

