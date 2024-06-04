/*
 * nsfcore.h
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

#ifndef NSFCORE_H
#define NSFCORE_H

#include <stdio.h>

typedef struct nsfcore_file_callb_t_
{
 ssize_t (*read)(struct nsfcore_file_callb_t_* callb, void* ptr, size_t count);
 int32 (*tell)(struct nsfcore_file_callb_t_* callb);
 int (*seek)(struct nsfcore_file_callb_t_* callb, int32 position);

 void* priv;
 uint32 priv_u32[2];
} nsfcore_file_callb_t;

typedef struct nsf_meta_t_
{
 uint8 song_count;
 uint8 song_start;
 uint8 chip;
 char title[127 + 1];
 char artist[127 + 1];
 char copyright[127 + 1];
 char song_titles[256][127 + 1];
 bool pal;
} nsf_meta_t;

//
// The contents of the returned static buffer are only valid after
// nsfcore_load(), nsfcore_load_scsp_bin(), or nsfcore_init() returns NULL
// or false to indicate an error, and before any further calls to
// those functions.
//
COLD_SECTION const char* nsfcore_get_error(void);

COLD_SECTION void nsfcore_make_mem_callb(nsfcore_file_callb_t* callb, const void* data, uint32 size);
COLD_SECTION void nsfcore_make_stdio_callb(nsfcore_file_callb_t* callb, FILE* stream);

COLD_SECTION bool nsfcore_init(void);

// Pointer returned from nsfcore_load*() will remain valid until 
// nsfcore_close() is called.
COLD_SECTION const nsf_meta_t* nsfcore_load(nsfcore_file_callb_t* callb);

// Call after nsfcore_load() and before nsfcore_set_song(), with callbacks
// to read either "scsppal.bin" or "scspntsc.bin", depending on the
// "pal" member of the nsf_meta_t struct.
COLD_SECTION bool nsfcore_load_scsp_bin(nsfcore_file_callb_t* callb);

void nsfcore_set_song(uint8 w);

// returns false if the new request was ignored due to a request already
// pending.
// Requires nsfcore_set_song() to be called at least once beforehand.
bool nsfcore_special(uint16 jsr_addr, uint8 accum_param);

// nsfcore_run_frame() must be called immediately(<1ms) after nsfcore_set_song(),
// and constantly thereafter(<1ms delay between calls) until nsfcore_close() or
// nsfcore_kill() is called.
//
// nsfcore_run_frame() disables interrupts via SR while the 6502 core
// is running.
//
// nsfcore_run_frame() blocks to synchronize with the SCSP and emulated APU,
// after the 6502 core is done running(with SR interrupt mask restored).
//
void nsfcore_run_frame(void);

// Special uses.
void* nsfcore_get_rom_ptr(size_t* max_size);

COLD_SECTION void nsfcore_close(void);
COLD_SECTION void nsfcore_kill(void);

#endif
