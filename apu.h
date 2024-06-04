/*
 * apu.h
 *
 * Copyright (C) 2024 celeriyacon - https://github.com/celeriyacon
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#ifndef CRONSF_APU_H
#define CRONSF_APU_H

COLD_SECTION void apu_preinit(void);
COLD_SECTION void apu_init(bool pal);
COLD_SECTION void apu_kill(void);
COLD_SECTION void apu_power(void);

void apu_start_sync(void);
void apu_write(uint32 timestamp, uint16 addr, uint8 value);
uint8 apu_read_4015(uint32 timestamp, uint16 addr, uint32 raw_data_bus_in);
void apu_force_update(uint32 timestamp);
void apu_frame(uint32 timestamp);

//void apu_pause(void);
//void apu_unpause(void);

#endif
