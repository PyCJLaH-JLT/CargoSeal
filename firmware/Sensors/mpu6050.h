#ifndef CS_MPU6050_H
#define CS_MPU6050_H
#include "register_bus.h"
typedef struct { float acceleration_g[3]; float rotation_dps[3]; float temperature_c; } cs_imu_sample;
typedef struct { cs_register_bus bus; uint8_t address; } cs_mpu6050;
cs_bus_result cs_mpu_start(cs_mpu6050 *device);
cs_bus_result cs_mpu_read(cs_mpu6050 *device, cs_imu_sample *sample, uint8_t raw[14]);
void cs_mpu_decode(const uint8_t raw[14], cs_imu_sample *sample);
#endif

