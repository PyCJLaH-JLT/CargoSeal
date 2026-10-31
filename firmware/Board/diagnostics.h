#ifndef CS_DIAGNOSTICS_H
#define CS_DIAGNOSTICS_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint32_t count, min, max, previous, late_max, missed, over_deadline;
    uint64_t sum;
    int32_t jitter_min, jitter_max;
    uint32_t histogram[5];
} cs_perf_stat;
void cs_perf_add(cs_perf_stat *,uint32_t);
void cs_perf_release(cs_perf_stat *,uint32_t now,uint32_t due,uint32_t period);
void cs_perf_init(void);
uint32_t cs_perf_now(void);
uint32_t cs_ms_to_us(uint32_t);
void cs_stack_paint(void);
unsigned cs_stack_used(void);
unsigned cs_heap_peak(void);
void cs_probe(unsigned pin,bool on);
#endif
