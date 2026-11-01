set pagination off
set confirm off
set remotetimeout 10
set logging file /work/gdb-erase.log
set logging overwrite on
set logging enabled on
file /work/firmware.elf
target remote host.docker.internal:50842
printf "INITIAL REGISTERS AND FLASH\n"
info registers
dump binary memory /work/nvm-before.bin 0x08007000 0x08008000
x/8wx 0x40022000
hbreak *0x08001468
commands
silent
printf "HAL ERASE status in r4=%u, PC=%#x\n", $r4, $pc
info registers
x/8wx 0x40022000
dump binary memory /work/nvm-at-hal-return.bin 0x08007000 0x08008000
detach
quit
end
shell touch /work/armed
continue
