#include "../firmware/Board/diagnostics.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    /* Stateless conversion is safe when an interrupt samples a later tick
       between two foreground samples. Unsigned deltas survive us wrap. */
    uint32_t before=cs_ms_to_us(4294967U);
    uint32_t nested=cs_ms_to_us(4294968U);
    uint32_t after=cs_ms_to_us(4294969U);
    assert(nested-before==1000U && after-nested==1000U);
    assert(cs_ms_to_us(0)==0 && cs_ms_to_us(UINT32_MAX)==(uint32_t)(UINT32_MAX*1000U));
    cs_perf_stat s={0};
    cs_perf_release(&s,0xfffff000U,0xfffff000U,20000);cs_perf_add(&s,400);
    uint32_t next=0xfffff000U+20000U;
    cs_perf_release(&s,next,next,20000);cs_perf_add(&s,800);
    cs_perf_release(&s,next+65000,next+20000,20000);cs_perf_add(&s,500);
    assert(s.count==3 && s.min==400 && s.max==800 && s.sum==1700);
    assert(s.missed==2 && s.late_max==45000 && s.jitter_max==45000 && s.jitter_min==0);
    assert(s.histogram[0]==1 && s.histogram[4]==1);
    puts("PASS: microsecond wrap, jitter histogram, release lateness, skipped slots, duration min/sum/max");
    return 0;
}
