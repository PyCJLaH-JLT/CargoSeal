#ifndef CS_IMU_BUS_H
#define CS_IMU_BUS_H
#include "../Sensors/register_bus.h"
cs_bus_result cs_board_imu_read(void *, uint8_t, uint8_t, uint8_t *, size_t);
cs_bus_result cs_board_imu_write(void *, uint8_t, uint8_t, const uint8_t *, size_t);
#endif

