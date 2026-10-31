#include "diagnostics.h"
uint32_t cs_ms_to_us(uint32_t ms){return ms*1000U;}
void cs_perf_add(cs_perf_stat *s,uint32_t us)
{
    if(!s->count || us<s->min)s->min=us;
    if(us>s->max)s->max=us;
    ++s->count;s->sum+=us;
}
void cs_perf_release(cs_perf_stat *s,uint32_t now,uint32_t due,uint32_t period)
{
    uint32_t late=now-due;
    if(late>s->late_max)s->late_max=late;
    s->missed+=late/period;
    if(s->count){
        int32_t j=(int32_t)(now-s->previous)-(int32_t)period;
        if(s->count==1 || j<s->jitter_min)s->jitter_min=j;
        if(s->count==1 || j>s->jitter_max)s->jitter_max=j;
        uint32_t magnitude=(uint32_t)(j<0?-j:j);
        unsigned bin=magnitude<100?0:magnitude<1000?1:magnitude<5000?2:magnitude<20000?3:4;
        ++s->histogram[bin];
    }
    s->previous=now;
}
#ifndef CS_HOST_TEST
#include "main.h"
extern uint8_t _estack;
extern uint32_t _Min_Stack_Size;
void cs_perf_init(void)
{
    GPIO_InitTypeDef pins={0};pins.Pin=GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8;
    pins.Mode=GPIO_MODE_OUTPUT_PP;pins.Pull=GPIO_NOPULL;pins.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA,&pins);
}
uint32_t cs_perf_now(void)
{ return cs_ms_to_us(HAL_GetTick()); }
void cs_probe(unsigned pin,bool on)
{ GPIOA->BSRR=1UL<<(pin+(on?0U:16U)); }
void cs_stack_paint(void)
{
    uint32_t *p=(uint32_t *)((uintptr_t)&_estack-(uintptr_t)&_Min_Stack_Size);
    uint32_t *limit=(uint32_t *)__get_MSP()-32;
    while(p<limit)*p++=0xa55ac33cU;
}
unsigned cs_stack_used(void)
{
    uint32_t *p=(uint32_t *)((uintptr_t)&_estack-(uintptr_t)&_Min_Stack_Size);
    while(p<(uint32_t *)&_estack && *p==0xa55ac33cU)++p;
    return (unsigned)((uintptr_t)&_estack-(uintptr_t)p);
}
#endif
