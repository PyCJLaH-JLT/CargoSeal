#include "log_medium.h"
#include "main.h"
#include <string.h>
#define LOG_BASE 0x08007000U
#define LOG_BYTES 4096U
static bool read_flash(void *ctx,unsigned offset,uint8_t *out,unsigned n)
{
    (void)ctx;if(offset>LOG_BYTES || n>LOG_BYTES-offset)return false;
    memcpy(out,(const void *)(LOG_BASE+offset),n);return true;
}
static bool program_flash(void *ctx,unsigned offset,const uint8_t *bytes)
{
    (void)ctx;
    if(offset>LOG_BYTES-8U || offset%8U)return false;
    uint64_t value;memcpy(&value,bytes,8);
    if(HAL_FLASH_Unlock()!=HAL_OK)return false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS|FLASH_FLAG_EOP);
    HAL_StatusTypeDef result=HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,LOG_BASE+offset,value);
    (void)HAL_FLASH_Lock();
    return result==HAL_OK;
}
static bool erase_flash(void *ctx)
{
    (void)ctx;
    if(HAL_FLASH_Unlock()!=HAL_OK)return false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS|FLASH_FLAG_EOP);
    FLASH_EraseInitTypeDef e={0};uint32_t error=0;
    e.TypeErase=FLASH_TYPEERASE_PAGES;e.Page=14;e.NbPages=2;
    HAL_StatusTypeDef result=HAL_FLASHEx_Erase(&e,&error);(void)HAL_FLASH_Lock();
    return result==HAL_OK;
}
cs_log_medium cs_board_log_medium(void)
{ return (cs_log_medium){NULL,read_flash,program_flash,erase_flash}; }
