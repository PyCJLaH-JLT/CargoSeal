#ifndef CS_PIR_H
#define CS_PIR_H
#include <stdbool.h>
typedef struct { bool high; } cs_pir;
static inline bool cs_pir_sample(cs_pir *s, bool high)
{
    bool rising = high && !s->high;
    s->high = high;
    return rising;
}
#endif

