#include "../firmware/Policy/container.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct { cs_event e[512];unsigned n; } logbook;
static void sink(void *ctx,const cs_event *e){logbook*l=ctx;assert(l->n<512);l->e[l->n++]=*e;}
static void fresh(cs_container*s,logbook*l,cs_mode m)
{ memset(l,0,sizeof*l);cs_container_init(s,sink,l);cs_container_mode(s,m,0);l->n=0;s->seq=0; }
static void sample(cs_container*s,float z,float gyro,uint32_t t)
{ cs_imu_sample v={{0,0,z},{gyro,0,0},24};cs_container_imu(s,&v,CS_BUS_OK,t); }
static void light(cs_container*s,int delta,uint32_t t)
{ cs_light l={.ready=true,.valid=true,.baseline=100,.average=(uint16_t)(100+delta)};cs_container_light(s,&l,t); }
int main(void)
{
 cs_container s;logbook l;
 for(unsigned mode=0;mode<2;++mode){
  fresh(&s,&l,(cs_mode)mode);
  sample(&s,1.3f,0,0);sample(&s,1.3f,0,199);assert(l.n==0);
  sample(&s,1.3f,0,200);assert(l.n==1);
  assert(l.e[0].kind==(mode?CS_MOVE:CS_DISTURBANCE) && s.alarm==!mode);
  sample(&s,1,0,220);sample(&s,1,0,719);sample(&s,1.3f,0,720);sample(&s,1.3f,0,920);
  assert(l.n==1);
  sample(&s,1,0,940);sample(&s,1,0,1440);sample(&s,1.3f,0,1460);sample(&s,1.3f,0,1660);
  assert(l.n==2);
  fresh(&s,&l,(cs_mode)mode);sample(&s,2.49f,0,0);sample(&s,1,0,40);assert(l.n==0);
  sample(&s,2.5f,0,60);assert(l.n==1 && l.e[0].kind==CS_IMPACT && s.alarm);
  sample(&s,2.51f,0,100);sample(&s,1.3f,0,120);sample(&s,1.3f,0,400);assert(l.n==1);
  sample(&s,1,0,420);sample(&s,1,0,920);sample(&s,2.51f,0,940);assert(l.n==2);
  fresh(&s,&l,(cs_mode)mode);sample(&s,1,4.99f,0);sample(&s,1,4.99f,200);assert(l.n==0);
  sample(&s,1,5,220);sample(&s,1,5,420);assert(l.n==1);
  fresh(&s,&l,(cs_mode)mode);
  light(&s,599,0);light(&s,599,500);assert(l.n==0);
  light(&s,600,550);light(&s,600,849);assert(l.n==0);light(&s,600,850);assert(l.n==1);
  light(&s,601,1000);light(&s,401,1100);light(&s,401,2000);light(&s,600,2300);assert(l.n==1);
  light(&s,400,2400);light(&s,400,2900);light(&s,601,2950);light(&s,601,3250);assert(l.n==2);
  fresh(&s,&l,(cs_mode)mode);
  light(&s,600,0);light(&s,0,299);light(&s,600,300);light(&s,600,599);assert(l.n==0);
  light(&s,600,600);assert(l.n==1);
 }
 fresh(&s,&l,CS_TRANSPORT);
 sample(&s,1.3f,0,100);cs_container_mode(&s,CS_INSTALLED,200);
 sample(&s,1.3f,0,280);sample(&s,1.3f,0,479);assert(l.n==1);
 sample(&s,1.3f,0,480);assert(l.n==2 && l.e[1].kind==CS_DISTURBANCE);
 fresh(&s,&l,CS_INSTALLED);
 light(&s,600,100);cs_container_pir(&s,true,400);sample(&s,2.5f,0,400);light(&s,600,400);
 assert(l.n==3 && l.e[0].kind==CS_NEARBY && l.e[1].kind==CS_IMPACT && l.e[2].kind==CS_LIGHT_CHANGE);
 assert(!cs_container_ack(&s));sample(&s,1,0,420);light(&s,0,450);
 sample(&s,1,0,920);light(&s,0,950);assert(cs_container_ack(&s));
 fresh(&s,&l,CS_TRANSPORT);cs_container_pir(&s,true,10);
 cs_container_mode(&s,CS_INSTALLED,20);cs_container_mode(&s,CS_INSTALLED,30);assert(l.n==2);
 assert(l.e[1].kind==CS_NEARBY);cs_container_pir(&s,true,40);assert(l.n==2);
 fresh(&s,&l,CS_INSTALLED);
 uint32_t start=UINT32_MAX-100U;sample(&s,1.3f,0,start);sample(&s,1.3f,0,98);assert(l.n==0);
 sample(&s,1.3f,0,99);assert(l.n==1);
 cs_imu_sample bad={{NAN,0,1},{0,0,0},24};cs_container_imu(&s,&bad,CS_BUS_OK,120);
 assert(!s.imu_valid && l.e[l.n-1].kind==CS_SENSOR_FAULT);
 sample(&s,1,0,140);assert(l.e[l.n-1].kind==CS_SENSOR_RECOVERED);
 for(unsigned i=1;i<l.n;++i)assert(l.e[i].seq>l.e[i-1].seq);
 fresh(&s,&l,CS_INSTALLED);
 light(&s,600,0);
 cs_light unavailable={0};
 cs_container_light(&s,&unavailable,100);
 light(&s,600,300);light(&s,600,599);assert(l.n==0);
 light(&s,600,600);assert(l.n==1);
 s.imu_valid=true;s.motion_normal=true;s.light_normal=true;
 s.light_valid=false;assert(!cs_container_ack(&s));s.light_valid=true;
 s.light_ready=false;assert(!cs_container_ack(&s));s.light_ready=true;
 s.motion_normal=false;assert(!cs_container_ack(&s));s.motion_normal=true;
 s.light_normal=false;assert(!cs_container_ack(&s));s.light_normal=true;
 assert(cs_container_ack(&s));
 assert(strcmp(cs_event_name(CS_IMPACT),"IMPACT")==0);
 assert(strcmp(cs_mode_name(CS_TRANSPORT),"TRANSPORT")==0);
 cs_container_init(&s,NULL,NULL);sample(&s,2.5f,0,0);assert(s.alarm && s.seq==1);
 puts("PASS: T03-T12 numerical boundaries, 200/300/500ms timers, simultaneous events, mode transitions, wrap, invalid data");
    return 0;
}

