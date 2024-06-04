#ifndef GS_BIOS_H
#define GS_BIOS_H

#define bios_change_clock_speed(n) ((*(void (*volatile*)(uint32))0x6000320)(n))
#define bios_slave_entry  (*(void (*volatile*)(void))0x06000250)

#endif
