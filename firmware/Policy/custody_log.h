#ifndef CS_CUSTODY_LOG_H
#define CS_CUSTODY_LOG_H
#include "container.h"
#define CS_LOG_RECORD_BYTES 40U
#define CS_LOG_SLOTS 102U
#define CS_LOG_QUEUE 8U
typedef struct {
    void *context;
    bool (*read)(void *, unsigned, uint8_t *, unsigned);
    bool (*program)(void *, unsigned, const uint8_t *);
    bool (*erase)(void *);
} cs_log_medium;
typedef struct {
    cs_log_medium medium;
    uint8_t pending[CS_LOG_QUEUE][CS_LOG_RECORD_BYTES];
    unsigned head, tail, queued, word, next_slot, valid, damaged;
    uint32_t boot, dropped, failures, committed;
    bool failed, full;
} cs_custody_log;
typedef struct {
    uint32_t boot, seq, time_ms;
    int32_t value, secondary;
    cs_mode mode;
    cs_event_kind kind;
    bool valid, alarm;
} cs_custody_record;
uint32_t cs_log_crc(const uint8_t *, unsigned);
unsigned cs_log_offset(unsigned slot);
bool cs_log_decode(const uint8_t *, cs_custody_record *);
void cs_log_open(cs_custody_log *, cs_log_medium);
bool cs_log_enqueue(cs_custody_log *, const cs_event *, bool alarm);
bool cs_log_step(cs_custody_log *);
bool cs_log_clear(cs_custody_log *);
typedef struct {
    unsigned checked, non_ff, first;
    uint8_t first_value;
} cs_medium_check;
cs_medium_check cs_log_check_medium(cs_custody_log *);
bool cs_log_get(cs_custody_log *, unsigned, cs_custody_record *);
#endif
