#include "../firmware/Policy/custody_log.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t flash[4096];
static unsigned cut=99999, programmed;
static bool lie;
static int bad_erase_offset=-1;
static bool rd(void *c,unsigned o,uint8_t *p,unsigned n)
{ (void)c;assert(o+n<=sizeof flash);memcpy(p,flash+o,n);return true; }
static bool wr(void *c,unsigned o,const uint8_t *p)
{
    (void)c;assert(o%8==0 && o+8<=sizeof flash);
    for(unsigned i=0;i<8;++i){
        if(programmed++==cut)return false;
        assert(flash[o+i]==0xff);if(!lie)flash[o+i]=p[i];
    }
    return true;
}
static bool erase(void *c){
    (void)c;memset(flash,0xff,sizeof flash);
    if(bad_erase_offset>=0)flash[bad_erase_offset]=0;
    return true;
}
static cs_log_medium medium={NULL,rd,wr,erase};
static cs_event impact={17,9821,CS_TRANSPORT,CS_IMPACT,1828,13000,true};
static void drain(cs_custody_log *s){while(cs_log_step(s) && !s->failed){}}
int main(void)
{
    assert(cs_log_crc((const uint8_t *)"123456789",9)==0xcbf43926U);
    cs_custody_log a,b;cs_custody_record r;
    /* Every byte position of five doublewords is a separate interrupted write. */
    for(unsigned boundary=0;boundary<=40;++boundary){
        erase(NULL);programmed=0;cut=99999;cs_log_open(&a,medium);
        assert(cs_log_enqueue(&a,&impact,true));drain(&a);
        assert(a.valid==1 && cs_log_get(&a,0,&r));
        assert(r.boot==1 && r.seq==17 && r.kind==CS_IMPACT && r.mode==CS_TRANSPORT && r.alarm);
        impact.seq=18;programmed=0;cut=boundary;
        assert(cs_log_enqueue(&a,&impact,true));drain(&a);cs_log_open(&b,medium);
        /* The last three bytes of ~18 are already erased FF. A prefix of 37
         * bytes therefore equals the complete intended record and is safe. */
        assert(b.valid==(boundary>=37?2U:1U));assert(cs_log_get(&b,0,&r));
        assert(b.boot==2 && r.seq==17);
        cut=99999;programmed=0;
        impact.seq=19;assert(cs_log_enqueue(&b,&impact,false));drain(&b);
        assert(cs_log_get(&b,b.next_slot-1,&r) && r.boot==2 && r.seq==19 && !r.alarm);
        impact.seq=17;
    }
    /* Single-bit corruptions of payload/CRC/commit must not become valid records. */
    erase(NULL);cs_log_open(&a,medium);assert(cs_log_enqueue(&a,&impact,true));drain(&a);
    for(unsigned byte=0;byte<40;++byte)for(unsigned bit=0;bit<8;++bit){
        flash[byte]^=(uint8_t)(1U<<bit);assert(!cs_log_get(&a,0,&r));flash[byte]^=(uint8_t)(1U<<bit);
    }
    /* A controller that reports success without changing memory is rejected. */
    erase(NULL);cs_log_open(&a,medium);lie=true;assert(cs_log_enqueue(&a,&impact,true));drain(&a);
    assert(a.failed && a.failures==1 && !a.committed);lie=false;
    assert(cs_log_clear(&a) && !a.failed && !a.queued);
    /* Check unused page tails too; scanning only records would miss these. */
    for(unsigned i=0;i<2;++i){
        bad_erase_offset=i?4095:2047;
        assert(!cs_log_clear(&a) && a.failed);
        cs_medium_check check=cs_log_check_medium(&a);
        assert(check.checked==4096 && check.non_ff==1 &&
               check.first==(unsigned)bad_erase_offset && check.first_value==0);
    }
    bad_erase_offset=-1;
    assert(cs_log_clear(&a) && !a.failed);
    cs_medium_check check=cs_log_check_medium(&a);
    assert(check.checked==4096 && !check.non_ff && check.first==4096);
    /* Full log preserves earlier custody evidence; queue overflow is explicit. */
    for(unsigned i=0;i<CS_LOG_QUEUE;++i)assert(cs_log_enqueue(&a,&impact,true));
    assert(!cs_log_enqueue(&a,&impact,true) && a.dropped==1);assert(!cs_log_clear(&a));drain(&a);
    for(unsigned i=CS_LOG_QUEUE;i<CS_LOG_SLOTS;++i){impact.seq=i+1;assert(cs_log_enqueue(&a,&impact,true));drain(&a);}
    assert(a.full && a.valid==102 && !cs_log_enqueue(&a,&impact,true));
    cs_log_open(&b,medium);assert(b.full && b.valid==102 && !b.damaged);
    assert(cs_log_get(&b,50,&r) && cs_log_get(&b,51,&r) && cs_log_get(&b,101,&r));
    puts("PASS: custody CRC vector, 41 cut positions, 320 bit corruptions, verify failure, queue, page crossing, 102-slot full, reboot");
    return 0;
}
