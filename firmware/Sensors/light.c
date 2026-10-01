#include "light.h"
bool cs_light_measure(cs_light *s, uint16_t raw, bool valid)
{
    s->valid = valid && raw <= 4095U;
    if (!s->valid) return false;
    s->raw = raw;
    uint16_t e = (uint16_t)(4095U - raw);
    s->sum -= s->window[s->position];
    s->window[s->position] = e;
    s->sum += e;
    s->position = (uint8_t)((s->position + 1U) % 4U);
    if (s->count < 4U) ++s->count;
    s->average = (uint16_t)(s->sum / s->count);
    if (!s->ready) {
        s->calibration_sum += e;
        if (++s->calibration_count == 20U) {
            s->baseline = (uint16_t)(s->calibration_sum / 20U);
            s->ready = true;
        }
    }
    return true;
}

