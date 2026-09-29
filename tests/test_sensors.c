#include "../firmware/Sensors/mpu6050.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct { uint8_t regs[128], frame[14]; unsigned calls, fail_at; cs_bus_result fail; } fake;
static cs_bus_result read_reg(void *ctx, uint8_t addr, uint8_t reg, uint8_t *out, size_t n)
{
    fake *f = ctx;
    assert(addr == 0x68);
    if (++f->calls == f->fail_at) return f->fail;
    memcpy(out, reg == 0x3B ? f->frame : f->regs + reg, n);
    return CS_BUS_OK;
}
static cs_bus_result write_reg(void *ctx, uint8_t addr, uint8_t reg, const uint8_t *in, size_t n)
{
    fake *f = ctx;
    assert(addr == 0x68 && n == 1);
    const uint8_t regs[] = {0x6B, 0x1A, 0x1B, 0x1C, 0x19};
    const uint8_t vals[] = {1, 3, 0, 0x10, 9};
    unsigned step = f->calls - 1U;
    assert(step < 5 && reg == regs[step] && *in == vals[step]);
    if (++f->calls == f->fail_at) return f->fail;
    memcpy(f->regs + reg, in, n);
    return CS_BUS_OK;
}
int main(void)
{
    fake f = {0}; f.regs[0x75] = 0x68;
    cs_mpu6050 d = {{&f, read_reg, write_reg}, 0x68};
    assert(cs_mpu_start(&d) == CS_BUS_OK && f.calls == 7);
    f.calls = 0; f.regs[0x75] = 0;
    assert(cs_mpu_start(&d) == CS_BUS_BAD_ID);
    f.regs[0x75] = 0x68;
    for (unsigned call = 1; call <= 7; ++call) {
        for (unsigned error = CS_BUS_NACK; error <= CS_BUS_TIMEOUT; ++error) {
            f.calls = 0; f.fail_at = call; f.fail = (cs_bus_result)error;
            assert(cs_mpu_start(&d) == (cs_bus_result)error);
            assert(f.calls == call);
        }
    }
    f.fail_at = 0;
    const uint8_t frame[14] = {0x10,0,0xF0,0,0x28,0,0xEF,0x5C,0,0x83,0xFF,0x7D,0,0};
    memcpy(f.frame, frame, 14);
    cs_imu_sample s = {0}; uint8_t raw[14];
    assert(cs_mpu_read(&d, &s, raw) == CS_BUS_OK);
    assert(s.acceleration_g[0] == 1 && s.acceleration_g[1] == -1 && s.acceleration_g[2] == 2.5f);
    assert(s.rotation_dps[0] == 1 && s.rotation_dps[1] == -1);
    assert(fabsf(s.temperature_c - 24.0f) < 0.01f);
    cs_imu_sample old = s; uint8_t oldraw[14]; memcpy(oldraw, raw, 14);
    f.fail_at = f.calls + 1; f.fail = CS_BUS_TIMEOUT;
    assert(cs_mpu_read(&d, &s, raw) == CS_BUS_TIMEOUT);
    assert(memcmp(&s, &old, sizeof s) == 0 && memcmp(raw, oldraw, 14) == 0);
    puts("PASS: MPU init ordering, range, signed units, NACK/timeout, stale output");
}

