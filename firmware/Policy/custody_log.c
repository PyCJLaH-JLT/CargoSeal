#include "custody_log.h"
#include <string.h>
static uint32_t load32(const uint8_t *p)
{ return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static void store32(uint8_t *p,uint32_t v)
{ for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8U*i)); }
uint32_t cs_log_crc(const uint8_t *p,unsigned n)
{
    uint32_t c=0xffffffffU;
    while(n--){c^=*p++;for(unsigned b=0;b<8;++b)c=(c>>1)^((0U-(c&1U))&0xedb88320U);}
    return ~c;
}
/* Each 2 KiB page holds 51 records; the unused eight bytes are never written. */
unsigned cs_log_offset(unsigned slot)
{ return (slot/51U)*2048U+(slot%51U)*CS_LOG_RECORD_BYTES; }
bool cs_log_decode(const uint8_t *p,cs_custody_record *r)
{
    if(load32(p)!=0x43534703U || load32(p+28)!=cs_log_crc(p,28) ||
       load32(p+32)!=0x5345414cU || load32(p+36)!=~load32(p+8) ||
       p[24]>CS_MODE_CHANGE || p[25]>CS_TRANSPORT || p[26]>1 || p[27]>1)return false;
    *r=(cs_custody_record){load32(p+4),load32(p+8),load32(p+12),
        (int32_t)load32(p+16),(int32_t)load32(p+20),(cs_mode)p[25],
        (cs_event_kind)p[24],p[26]!=0,p[27]!=0};
    return true;
}
void cs_log_open(cs_custody_log *s,cs_log_medium m)
{
    memset(s,0,sizeof *s);s->medium=m;
    uint8_t bytes[CS_LOG_RECORD_BYTES];cs_custody_record record;uint32_t last_boot=0;
    for(unsigned slot=0;slot<CS_LOG_SLOTS;++slot){
        if(!m.read(m.context,cs_log_offset(slot),bytes,sizeof bytes)){s->failed=true;++s->failures;break;}
        bool blank=true;for(unsigned i=0;i<sizeof bytes;++i)if(bytes[i]!=0xff)blank=false;
        if(blank)continue;
        s->next_slot=slot+1;
        if(cs_log_decode(bytes,&record)){
            ++s->valid;if(record.boot>last_boot)last_boot=record.boot;
        }else ++s->damaged;
    }
    s->boot=last_boot+1U;s->full=s->next_slot>=CS_LOG_SLOTS;
}
bool cs_log_enqueue(cs_custody_log *s,const cs_event *e,bool alarm)
{
    if(s->failed || s->next_slot+s->queued>=CS_LOG_SLOTS || s->queued==CS_LOG_QUEUE){++s->dropped;return false;}
    uint8_t *p=s->pending[s->head];
    store32(p,0x43534703U);store32(p+4,s->boot);store32(p+8,e->seq);store32(p+12,e->time_ms);
    store32(p+16,(uint32_t)e->value);store32(p+20,(uint32_t)e->secondary);
    p[24]=(uint8_t)e->kind;p[25]=(uint8_t)e->mode;p[26]=(uint8_t)e->valid;p[27]=(uint8_t)alarm;
    store32(p+28,cs_log_crc(p,28));store32(p+32,0x5345414cU);store32(p+36,~e->seq);
    s->head=(s->head+1U)%CS_LOG_QUEUE;++s->queued;return true;
}
bool cs_log_step(cs_custody_log *s)
{
    if(s->failed || !s->queued)return false;
    unsigned offset=cs_log_offset(s->next_slot)+s->word*8U;
    uint8_t verify[8];const uint8_t *p=s->pending[s->tail]+s->word*8U;
    /* Verify every doubleword, including the final commit, before acknowledging. */
    if(!s->medium.program(s->medium.context,offset,p) ||
       !s->medium.read(s->medium.context,offset,verify,8) || memcmp(p,verify,8)){
        s->failed=true;++s->failures;return true;
    }
    if(++s->word==5U){
        s->word=0;s->tail=(s->tail+1U)%CS_LOG_QUEUE;--s->queued;
        ++s->next_slot;++s->valid;++s->committed;s->full=s->next_slot==CS_LOG_SLOTS;
    }
    return true;
}
cs_medium_check cs_log_check_medium(cs_custody_log *s)
{
    cs_medium_check check={0,0,4096,0xff};uint8_t bytes[32];
    for(unsigned offset=0;offset<4096;offset+=sizeof bytes){
        if(!s->medium.read(s->medium.context,offset,bytes,sizeof bytes))break;
        for(unsigned i=0;i<sizeof bytes;++i)if(bytes[i]!=0xff){
            if(!check.non_ff){check.first=offset+i;check.first_value=bytes[i];}
            ++check.non_ff;
        }
        check.checked+=sizeof bytes;
    }
    return check;
}
bool cs_log_clear(cs_custody_log *s)
{
    if(s->queued && !s->failed)return false;
    if(!s->medium.erase(s->medium.context)){s->failed=true;++s->failures;return false;}
    cs_medium_check check=cs_log_check_medium(s);
    if(check.checked!=4096 || check.non_ff){s->failed=true;++s->failures;return false;}
    cs_log_medium m=s->medium;cs_log_open(s,m);
    return !s->failed && s->next_slot==0;
}
bool cs_log_get(cs_custody_log *s,unsigned slot,cs_custody_record *r)
{
    uint8_t bytes[CS_LOG_RECORD_BYTES];
    return slot<CS_LOG_SLOTS && s->medium.read(s->medium.context,cs_log_offset(slot),bytes,sizeof bytes)
        && cs_log_decode(bytes,r);
}
