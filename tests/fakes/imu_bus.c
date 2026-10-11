#include "main.h"
#include "../../firmware/Board/imu_bus.h"
extern I2C_HandleTypeDef hi2c1;
static cs_bus_result convert(HAL_StatusTypeDef r)
{
    if(r==HAL_OK)return CS_BUS_OK;
    if(r==HAL_TIMEOUT)return CS_BUS_TIMEOUT;
    return CS_BUS_NACK;
}
cs_bus_result cs_board_imu_read(void *ctx,uint8_t address,uint8_t reg,uint8_t *out,size_t n)
{
    (void)ctx;
    return convert(HAL_I2C_Mem_Read(&hi2c1,(uint16_t)(address<<1),reg,1,out,(uint16_t)n,10));
}
cs_bus_result cs_board_imu_write(void *ctx,uint8_t address,uint8_t reg,const uint8_t *data,size_t n)
{
    (void)ctx;
    return convert(HAL_I2C_Mem_Write(&hi2c1,(uint16_t)(address<<1),reg,1,(uint8_t *)data,(uint16_t)n,10));
}

