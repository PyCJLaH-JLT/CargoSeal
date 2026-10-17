#ifndef CS_COMMANDS_H
#define CS_COMMANDS_H
#include <stdbool.h>
#include <stddef.h>
typedef enum { CS_CMD_ERROR, CS_CMD_STATUS, CS_CMD_ACK, CS_CMD_TRANSPORT, CS_CMD_INSTALLED,
               CS_CMD_SELFTEST, CS_CMD_READ_IMU, CS_CMD_ADDRESS_68, CS_CMD_ADDRESS_69 } cs_command;
typedef struct { char line[64]; size_t length; bool overflow; } cs_line_reader;
cs_command cs_command_parse(const char *);
bool cs_line_byte(cs_line_reader *, unsigned char, cs_command *);
#endif

