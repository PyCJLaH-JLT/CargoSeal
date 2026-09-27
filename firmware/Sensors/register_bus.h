#ifndef CS_REGISTER_BUS_H
#define CS_REGISTER_BUS_H
#include <stddef.h>
#include <stdint.h>
typedef enum { CS_BUS_OK, CS_BUS_NACK, CS_BUS_TIMEOUT, CS_BUS_BAD_ID, CS_BUS_CONFIG } cs_bus_result;
typedef struct {
    void *context;
    cs_bus_result (*read)(void *, uint8_t address, uint8_t reg, uint8_t *, size_t);
    cs_bus_result (*write)(void *, uint8_t address, uint8_t reg, const uint8_t *, size_t);
} cs_register_bus;
#endif

