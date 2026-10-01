#ifndef CS_LIGHT_H
#define CS_LIGHT_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint16_t window[4], raw, average, baseline;
    uint32_t sum, calibration_sum;
    uint8_t count, position, calibration_count;
    bool ready, valid;
} cs_light;
bool cs_light_measure(cs_light *s, uint16_t raw, bool valid);
#endif

