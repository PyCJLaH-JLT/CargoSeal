#include "../firmware/Policy/container.h"
#include <assert.h>
#include <stdio.h>
typedef struct { cs_event records[128]; unsigned count; } logbook;
static void collect(void *ctx, const cs_event *event)
{ logbook *l = ctx; assert(l->count < 128); l->records[l->count++] = *event; }
int main(void)
{
    cs_container s; logbook l = {0}; cs_container_init(&s, collect, &l);
    cs_imu_sample imu = {{0, 0, 1.3f}, {0, 0, 0}, 24};
    for (unsigned t=0; t<200; t+=20) cs_container_imu(&s, &imu, CS_BUS_OK, t);
    assert(l.count == 0);
    cs_container_imu(&s, &imu, CS_BUS_OK, 200);
    assert(l.count == 1 && l.records[0].kind == CS_DISTURBANCE && s.alarm);
    imu.acceleration_g[2] = 2.5f;
    cs_container_imu(&s, &imu, CS_BUS_OK, 220);
    assert(l.count == 2 && l.records[1].kind == CS_IMPACT);
    for (unsigned t=240; t<=1000; t+=20) cs_container_imu(&s, &imu, CS_BUS_OK, t);
    assert(l.count == 2);
    imu.acceleration_g[2] = 1;
    for (unsigned t=1020; t<=1520; t+=20) cs_container_imu(&s, &imu, CS_BUS_OK, t);
    imu.acceleration_g[2] = 2.49f;
    cs_container_imu(&s, &imu, CS_BUS_OK, 1540); assert(l.count == 2);
    imu.acceleration_g[2] = 2.5f;
    cs_container_imu(&s, &imu, CS_BUS_OK, 1560); assert(l.count == 3);
    imu.acceleration_g[2] = 1.3f;
    for (unsigned t=1580; t<=1900; t+=20) cs_container_imu(&s, &imu, CS_BUS_OK, t);
    assert(l.count == 3);
    cs_container_imu(&s, &imu, CS_BUS_TIMEOUT, 1920);
    cs_container_imu(&s, &imu, CS_BUS_NACK, 1940);
    assert(l.count == 4 && !s.imu_valid);
    cs_container_pir(&s, true, 1940); cs_container_pir(&s, true, 1960);
    assert(l.count == 5 && l.records[4].kind == CS_NEARBY);
    cs_light light = {.ready=true,.valid=true,.average=700,.baseline=100};
    for (unsigned t=2000; t<=2300; t+=50) cs_container_light(&s,&light,t);
    assert(l.count == 6 && l.records[5].kind == CS_LIGHT_CHANGE);
    cs_container_imu(&s, &imu, CS_BUS_OK, 2320);
    assert(l.records[6].kind == CS_SENSOR_RECOVERED);
    for(unsigned i=0;i<l.count;++i) assert(l.records[i].seq == i+1);
    puts("PASS: motion confirmation, impact boundary/priority, fault/recovery, independent PIR/light");
}

