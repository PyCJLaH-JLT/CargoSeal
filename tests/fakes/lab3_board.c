#include "main.h"
#include "../../firmware/Board/diagnostics.h"
#include "../../firmware/Board/log_medium.h"
#include <string.h>
static uint8_t flash[4096];
static bool initialized;
void (*cs_fake_erase_hook)(void);
uint32_t cs_perf_now(void){return cs_ms_to_us(HAL_GetTick());}
void cs_perf_init(void){}
void cs_probe(unsigned p,bool on){(void)p;(void)on;}
void cs_stack_paint(void){}
unsigned cs_stack_used(void){return 0;}
unsigned cs_heap_peak(void){return 0;}
static bool read_medium(void *c,unsigned offset,uint8_t *p,unsigned n)
{ (void)c;if(offset+n>sizeof flash)return false;memcpy(p,flash+offset,n);return true; }
static bool program_medium(void *c,unsigned offset,const uint8_t *p)
{ (void)c;for(unsigned i=0;i<8;++i){if(flash[offset+i]!=0xff)return false;flash[offset+i]=p[i];}return true; }
static bool erase_medium(void *c)
{ (void)c;if(cs_fake_erase_hook)cs_fake_erase_hook();memset(flash,0xff,sizeof flash);return true; }
cs_log_medium cs_board_log_medium(void)
{
    if(!initialized){memset(flash,0xff,sizeof flash);initialized=true;}
    return (cs_log_medium){NULL,read_medium,program_medium,erase_medium};
}
