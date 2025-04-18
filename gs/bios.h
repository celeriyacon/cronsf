#ifndef GIGASANTA_BIOS_H
#define GIGASANTA_BIOS_H

#define bios_change_clock_speed(n) ((*(void (*volatile*)(uint32))0x6000320)(n))
#define bios_slave_entry (*(void (*volatile*)(void))0x06000250)
#define bios_run_cd_player (*(void (*volatile*)(void))0x0600026C)

#define bios_read_first_mode (*(volatile uint32*)0x06000290)

#endif
