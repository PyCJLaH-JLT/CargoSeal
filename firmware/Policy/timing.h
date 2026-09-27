#ifndef CS_TIMING_H
#define CS_TIMING_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint32_t since; bool running; } cs_timer;
static inline bool cs_elapsed(uint32_t now, uint32_t since, uint32_t delay)
{ return (uint32_t)(now - since) >= delay; }
static inline bool cs_confirm(cs_timer *t, bool condition, uint32_t now, uint32_t delay)
{
    if (!condition) { t->running = false; return false; }
    if (!t->running) { t->running = true; t->since = now; }
    return cs_elapsed(now, t->since, delay);
}
#endif

