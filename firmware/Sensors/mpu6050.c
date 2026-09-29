#include "mpu6050.h"
static int32_t signed_be(const uint8_t *p)
{
    uint32_t value = ((uint32_t)p[0] << 8) | p[1];
    return value < 32768U ? (int32_t)value : (int32_t)value - 65536;
}
void cs_mpu_decode(const uint8_t raw[14], cs_imu_sample *s)
{
    for (unsigned i = 0; i < 3; ++i) {
        s->acceleration_g[i] = (float)signed_be(raw + 2U * i) / 4096.0f;
        s->rotation_dps[i] = (float)signed_be(raw + 8U + 2U * i) / 131.0f;
    }
    s->temperature_c = (float)signed_be(raw + 6) / 340.0f + 36.53f;
}
cs_bus_result cs_mpu_start(cs_mpu6050 *d)
{
    uint8_t id;
    cs_bus_result r = d->bus.read(d->bus.context, d->address, 0x75, &id, 1);
    if (r != CS_BUS_OK) return r;
    if (id != 0x68) return CS_BUS_BAD_ID;
    const uint8_t registers[] = {0x6B, 0x1A, 0x1B, 0x1C, 0x19};
    const uint8_t values[] = {0x01, 0x03, 0x00, 0x10, 0x09};
    for (unsigned i = 0; i < sizeof registers; ++i) {
        r = d->bus.write(d->bus.context, d->address, registers[i], &values[i], 1);
        if (r != CS_BUS_OK) return r;
    }
    uint8_t ranges[2];
    r = d->bus.read(d->bus.context, d->address, 0x1B, ranges, 2);
    if (r != CS_BUS_OK) return r;
    if ((ranges[0] & 0x18U) != 0 || (ranges[1] & 0x18U) != 0x10U) return CS_BUS_CONFIG;
    return CS_BUS_OK;
}
cs_bus_result cs_mpu_read(cs_mpu6050 *d, cs_imu_sample *sample, uint8_t raw[14])
{
    uint8_t bytes[14];
    cs_bus_result r = d->bus.read(d->bus.context, d->address, 0x3B, bytes, sizeof bytes);
    if (r != CS_BUS_OK) return r;
    cs_mpu_decode(bytes, sample);
    for (unsigned i = 0; i < sizeof bytes; ++i) raw[i] = bytes[i];
    return CS_BUS_OK;
}

