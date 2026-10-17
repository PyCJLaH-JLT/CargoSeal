#ifndef CS_CONTAINER_H
#define CS_CONTAINER_H
#include "timing.h"
#include "../Sensors/mpu6050.h"
#include "../Sensors/light.h"
#include "../Sensors/pir.h"
typedef enum { CS_INSTALLED, CS_TRANSPORT } cs_mode;
typedef enum { CS_MOVE, CS_DISTURBANCE, CS_IMPACT, CS_NEARBY, CS_LIGHT_CHANGE,
               CS_SENSOR_FAULT, CS_SENSOR_RECOVERED, CS_MODE_CHANGE } cs_event_kind;
typedef struct {
    uint32_t seq, time_ms;
    cs_mode mode;
    cs_event_kind kind;
    int32_t value, secondary;
    bool valid;
} cs_event;
typedef void (*cs_event_sink)(void *, const cs_event *);
typedef struct {
    cs_mode mode;
    bool alarm, imu_valid, fault_announced, motion_reported, impact_reported;
    bool light_reported, light_valid, light_ready, motion_normal, light_normal;
    cs_pir pir;
    uint32_t seq;
    float deviation_g, rotation_dps;
    int32_t light_delta;
    cs_timer motion_confirm, motion_quiet, impact_quiet, light_confirm, light_quiet;
    cs_event_sink sink;
    void *sink_context;
} cs_container;
void cs_container_init(cs_container *, cs_event_sink, void *);
void cs_container_imu(cs_container *, const cs_imu_sample *, cs_bus_result, uint32_t);
void cs_container_pir(cs_container *, bool high, uint32_t);
void cs_container_light(cs_container *, const cs_light *, uint32_t);
void cs_container_mode(cs_container *, cs_mode, uint32_t);
bool cs_container_ack(cs_container *);
const char *cs_mode_name(cs_mode);
const char *cs_event_name(cs_event_kind);
#endif

