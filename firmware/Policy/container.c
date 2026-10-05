#include "container.h"
#include <math.h>
#include <string.h>
static void emit(cs_container *s, cs_event_kind kind, int32_t value, int32_t secondary,
                 bool valid, uint32_t now)
{
    cs_event event = {++s->seq, now, s->mode, kind, value, secondary, valid};
    if (s->sink) s->sink(s->sink_context, &event);
}
void cs_container_init(cs_container *s, cs_event_sink sink, void *ctx)
{
    memset(s, 0, sizeof *s);
    s->mode = CS_INSTALLED;
    s->sink = sink; s->sink_context = ctx;
}
const char *cs_mode_name(cs_mode mode)
{ return mode == CS_TRANSPORT ? "TRANSPORT" : "INSTALLED"; }
const char *cs_event_name(cs_event_kind kind)
{
    static const char *const names[] = {"MOVE", "DISTURBANCE", "IMPACT", "NEARBY",
        "LIGHT_CHANGE", "SENSOR_FAULT", "SENSOR_RECOVERED", "MODE_CHANGE"};
    return names[kind];
}
void cs_container_imu(cs_container *s, const cs_imu_sample *sample, cs_bus_result result,
                      uint32_t now)
{
    if (result != CS_BUS_OK) {
        s->imu_valid = false; s->motion_normal = false;
        s->motion_confirm.running = s->motion_quiet.running = s->impact_quiet.running = false;
        if (!s->fault_announced) {
            s->fault_announced = true;
            emit(s, CS_SENSOR_FAULT, (int32_t)result, 0, false, now);
        }
        return;
    }
    float a2 = 0, g2 = 0;
    for (unsigned i = 0; i < 3; ++i) {
        if (!isfinite(sample->acceleration_g[i]) || !isfinite(sample->rotation_dps[i])) {
            cs_container_imu(s, sample, CS_BUS_CONFIG, now);
            return;
        }
        a2 += sample->acceleration_g[i] * sample->acceleration_g[i];
        g2 += sample->rotation_dps[i] * sample->rotation_dps[i];
    }
    s->imu_valid = true;
    if (s->fault_announced) {
        s->fault_announced = false;
        emit(s, CS_SENSOR_RECOVERED, 0, 0, true, now);
    }
    s->deviation_g = fabsf(sqrtf(a2) - 1.0f);
    s->rotation_dps = sqrtf(g2);
    bool moving = s->deviation_g >= 0.10f || s->rotation_dps >= 5.0f;
    bool impact = s->deviation_g >= 1.50f;
    s->motion_normal = cs_confirm(&s->motion_quiet, !moving, now, 500);
    if (s->motion_normal) s->motion_reported = false;
    if (cs_confirm(&s->impact_quiet, !impact, now, 500)) s->impact_reported = false;
    if (impact) {
        s->motion_confirm.running = false;
        s->motion_reported = true;
        if (!s->impact_reported) {
            s->impact_reported = true; s->alarm = true;
            emit(s, CS_IMPACT, (int32_t)(s->deviation_g * 1000.0f + 0.5f),
                 (int32_t)(s->rotation_dps * 1000.0f + 0.5f), true, now);
        }
    } else if (!s->motion_reported && cs_confirm(&s->motion_confirm, moving, now, 200)) {
        s->motion_reported = true;
        if (s->mode == CS_INSTALLED) s->alarm = true;
        emit(s, s->mode == CS_TRANSPORT ? CS_MOVE : CS_DISTURBANCE,
             (int32_t)(s->deviation_g * 1000.0f + 0.5f),
             (int32_t)(s->rotation_dps * 1000.0f + 0.5f), true, now);
    } else if (!moving) s->motion_confirm.running = false;
}
void cs_container_pir(cs_container *s, bool high, uint32_t now)
{
    if (cs_pir_sample(&s->pir, high) && s->mode == CS_INSTALLED)
        emit(s, CS_NEARBY, 1, 0, true, now);
}
void cs_container_mode(cs_container *s, cs_mode mode, uint32_t now)
{
    if (mode == s->mode) return;
    cs_mode previous = s->mode;
    s->mode = mode;
    s->motion_confirm.running = s->motion_quiet.running = s->impact_quiet.running = false;
    s->motion_normal = false;
    emit(s, CS_MODE_CHANGE, (int32_t)previous, (int32_t)mode, true, now);
    if (mode == CS_INSTALLED && s->pir.high) emit(s, CS_NEARBY, 1, 0, true, now);
}
bool cs_container_ack(cs_container *s)
{
    if (!s->imu_valid || !s->light_valid || !s->light_ready ||
        !s->motion_normal || !s->light_normal) return false;
    s->alarm = false;
    return true;
}
void cs_container_light(cs_container *s, const cs_light *light, uint32_t now)
{
    s->light_valid = light->valid; s->light_ready = light->ready;
    if (!light->valid || !light->ready) {
        s->light_confirm.running = s->light_quiet.running = false;
        s->light_normal = false;
        return;
    }
    s->light_delta = (int32_t)light->average - light->baseline;
    s->light_normal = cs_confirm(&s->light_quiet, s->light_delta <= 400, now, 500);
    if (s->light_normal) s->light_reported = false;
    if (!s->light_reported && cs_confirm(&s->light_confirm, s->light_delta >= 600, now, 300)) {
        s->light_reported = true; s->alarm = true;
        emit(s, CS_LIGHT_CHANGE, s->light_delta, light->baseline, true, now);
    } else if (s->light_delta < 600) s->light_confirm.running = false;
}

