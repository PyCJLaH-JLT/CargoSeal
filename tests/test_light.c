#include "../firmware/Sensors/light.h"
#include "../firmware/Sensors/pir.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    cs_light s = {0};
    assert(!cs_light_measure(&s, 4096, true) && !s.valid);
    assert(!cs_light_measure(&s, 0, false));
    for (unsigned i = 0; i < 19; ++i) assert(cs_light_measure(&s, 4000, true) && !s.ready);
    assert(cs_light_measure(&s, 4000, true) && s.ready && s.baseline == 95);
    assert(cs_light_measure(&s, 1600, true) && s.average == 695);
    assert(cs_light_measure(&s, 1600, true) && s.average == 1295);
    assert(cs_light_measure(&s, 1600, true) && s.average == 1895);
    assert(cs_light_measure(&s, 1600, true) && s.average == 2495);
    assert(s.baseline == 95);
    for (unsigned i = 0; i < 4; ++i) cs_light_measure(&s, 4095, true);
    assert(s.average == 0);
    for (unsigned i = 0; i < 4; ++i) cs_light_measure(&s, 0, true);
    assert(s.average == 4095);
    cs_pir pir = {0};
    assert(cs_pir_sample(&pir, true));
    assert(!cs_pir_sample(&pir, true));
    assert(!cs_pir_sample(&pir, false));
    assert(cs_pir_sample(&pir, true));
    puts("PASS: ADC endpoints, polarity, four samples, twenty sample baseline, PIR edges");
}

