#include "../firmware/Policy/container.h"
#include "../firmware/Policy/commands.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned nearby, changes;
static void collect(void *ctx, const cs_event *e)
{ (void)ctx; nearby += e->kind == CS_NEARBY; changes += e->kind == CS_MODE_CHANGE; }
int main(void)
{
    cs_container s; cs_container_init(&s,collect,NULL);
    assert(s.mode == CS_INSTALLED);
    cs_container_mode(&s,CS_TRANSPORT,10); cs_container_pir(&s,true,20);
    assert(nearby == 0);
    s.alarm=true; s.light_delta=700; s.light_ready=true;
    s.motion_confirm=(cs_timer){10,true};
    cs_container_mode(&s,CS_INSTALLED,30);
    assert(nearby == 1 && changes == 2 && s.alarm && s.light_delta == 700 && !s.motion_confirm.running);
    cs_container_mode(&s,CS_INSTALLED,40); assert(nearby == 1 && changes == 2);
    assert(!cs_container_ack(&s));
    s.imu_valid=s.light_valid=s.light_ready=s.motion_normal=s.light_normal=true;
    assert(cs_container_ack(&s) && !s.alarm);
    cs_line_reader reader={0}; cs_command cmd=CS_CMD_ERROR;
    const char *line="MODE TRANSPORT";
    for(size_t i=0;i<strlen(line);++i) assert(!cs_line_byte(&reader,(unsigned char)line[i],&cmd));
    assert(cs_line_byte(&reader,'\n',&cmd) && cmd==CS_CMD_TRANSPORT);
    const char *commands[]={"STATUS","ACK","MODE TRANSPORT","MODE INSTALLED","SELFTEST","READ IMU","IMU ADDRESS 0x68","IMU ADDRESS 0x69"};
    for(unsigned i=0;i<8;++i) assert(cs_command_parse(commands[i])==(cs_command)(i+1));
    assert(cs_command_parse("MODE transport")==CS_CMD_ERROR);
    for(unsigned i=0;i<100;++i) cs_line_byte(&reader,'A',&cmd);
    assert(cs_line_byte(&reader,'\n',&cmd) && cmd==CS_CMD_ERROR);
    for(unsigned i=0;i<6;++i) cs_line_byte(&reader,(unsigned char)"STATUS"[i],&cmd);
    cs_line_byte(&reader,'\r',&cmd);
    assert(cs_line_byte(&reader,'\n',&cmd) && cmd==CS_CMD_STATUS);
    assert(cs_line_byte(&reader,'\n',&cmd) && cmd==CS_CMD_ERROR);
    cs_line_byte(&reader,0,&cmd);
    assert(cs_line_byte(&reader,'\n',&cmd) && cmd==CS_CMD_ERROR);
    puts("PASS: explicit modes, HIGH PIR transition, persistent alarm, ACK guard, LF parser and overflow");
    return 0;
}

