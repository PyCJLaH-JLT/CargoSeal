#include "commands.h"
#include <string.h>
cs_command cs_command_parse(const char *line)
{
    static const char *const strings[] = {"STATUS","ACK","MODE TRANSPORT","MODE INSTALLED",
        "SELFTEST","READ IMU","IMU ADDRESS 0x68","IMU ADDRESS 0x69"};
    for (unsigned i=0;i<sizeof strings/sizeof strings[0];++i)
        if (strcmp(line,strings[i])==0) return (cs_command)(i+1U);
    return CS_CMD_ERROR;
}
bool cs_line_byte(cs_line_reader *s, unsigned char byte, cs_command *out)
{
    if (byte == '\n') {
        if (s->length && s->line[s->length-1U]=='\r') --s->length;
        s->line[s->length]='\0';
        *out = s->overflow ? CS_CMD_ERROR : cs_command_parse(s->line);
        s->length=0; s->overflow=false;
        return true;
    }
    if (s->length >= sizeof s->line-1U || byte == 0) s->overflow=true;
    else if (!s->overflow) s->line[s->length++]=(char)byte;
    return false;
}

