/*
 * nsfcore.c
 *
 * Copyright (C) 2024-2025 celeriyacon - https://github.com/celeriyacon
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

#include "config.h"

#include "gs/types.h"
#include "gs/endian.h"
#include "gs/scsp.h"
#include "gs/sh2.h"

#include "nsfcore.h"
#include "apu.h"
#include "s6502.h"
#include "vrc6.h"
#include "sun5b.h"
#include "n163.h"
#include "mmc5.h"

#include "exchip.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#if EXCHIP_BENCH
 #include <unistd.h>
#endif

#define FOURCC(a, b, c, d) (((uint32)a << 24) | ((uint32)b << 16)  | (c << 8) | (d << 0))

ALIGN(256) static uint8 rom[NSFCORE_MAX_ROM_SIZE];
ALIGN(256) static uint8 shim_6502[0x1000] =
{
 #include "shim_6502.bin.h"
};

ALIGN(256) static uint8 prgram[0x2000];

static uint32 time_fract;
static int8 frame_alt;

static uint16 nsf_init_addr;
static uint16 nsf_play_addr;
static uint8 nsf_song_number;

static uint8 nsf_initial_banks[8];
static bool nsf_bs_enabled;

enum
{
 MIX_LEVEL_APU_SQUARE = 0x0,
 MIX_LEVEL_VRC6 = 0x2,
 MIX_LEVEL_FDS = 0x4,
 MIX_LEVEL_MMC5 = 0x5,
 MIX_LEVEL_N163 = 0x6,
 MIX_LEVEL_SUN5B = 0x7
};
static int32 nsf_mix_levels[8];

static void (*exchip_slave_entry)(uint32, volatile uint16*, uint32) = NULL;
static uint32 exchip_volume;

__attribute__((section(".loram.bss.uncached.exchip_rb"))) volatile uint32 exchip_rb[16384];

exchip_tables_t exchip_tables LORAM_BSS_UNCACHED;

int16 exchip_resamp[256 * 40] __attribute__((aligned(16)));

HIRAM_BSS_UNCACHED volatile uint32 exchip_bench[8];
HIRAM_BSS_UNCACHED volatile uint32 exchip_bench_rb_wr;
static uint32 exchip_bench_rb_rd;

#if EXCHIP_SINESWEEP
const int16 exchip_sintab[4096] =
{
 #include "sin.h"
};
#endif

VDP2_BSS static nsf_meta_t meta;
VDP2_BSS static char error_message[128];

COLD_SECTION const char* nsfcore_get_error(void)
{
 return error_message;
}

COLD_SECTION void* nsfcore_get_rom_ptr(size_t* max_size)
{
 *max_size = sizeof(rom);

 return rom;
}

void nsf_read_frame_pending_S(void);
void nsf_set_bank_S(void);

static const uint8 iso88591_to_cp850[256] =
{
 #include "iso88591_to_cp850.h"
};

COLD_SECTION static bool read_nsfe_string(nsfcore_file_callb_t* callb, char* s, const size_t s_size)
{
 char c;
 size_t i = 0;

 for(;;)
 {
  if(callb->read(callb, &c, 1) != 1)
   return false;

  if(i < s_size)
  {
   s[i] = iso88591_to_cp850[(unsigned char)c];
   i++;
  }

  if(!c)
   break;
 }

 s[s_size - 1] = 0;

 return true;
}

COLD_SECTION static const nsf_meta_t* nsfe_load(nsfcore_file_callb_t* callb)
{
 uint8 file_header[4];

 if(callb->read(callb, file_header, 4) != 4)
 {
  snprintf(error_message, sizeof(error_message), "Error reading file header.");
  return NULL;
 }

 if(memcmp(file_header, "NSFE", 4))
 {
  snprintf(error_message, sizeof(error_message), "NSFe file header magic string not present.");
  return NULL;
 }
 //
 //
 uint16 nsf_load_addr = 0;
 uint8 header[8];

 while(callb->read(callb, header, sizeof(header)) == sizeof(header))
 {
  const uint32 chunk_data_start = callb->tell(callb);
  const uint32 chunk_id = read32_be(header + 4);
  const uint32 chunk_size = read32_le(header + 0);
  //

  switch(chunk_id)
  {
   default:
	if(chunk_id >= ('A' << 24) && (chunk_id <= 'Z' << 24))
	{
	 snprintf(error_message, sizeof(error_message), "Unsupported mandatory \"%.4s\" chunk.", (char*)header + 4);
	 return NULL;
	}

	break;

   case FOURCC('I', 'N', 'F', 'O'):
	{
	 uint8 buf[0xA] = { 0 };

         if(chunk_size < 9)
	 {
	  snprintf(error_message, sizeof(error_message), "NSFe \"%s\" chunk size is wrong.", "INFO");
          return NULL;
	 }

	 if(callb->read(callb, buf, sizeof(buf)) != sizeof(buf))
         {
	  snprintf(error_message, sizeof(error_message), "Error reading NSFe \"%s\" chunk.", "INFO");
          return NULL;
         }

	 nsf_load_addr = read16_le(buf + 0x00);
	 nsf_init_addr = read16_le(buf + 0x02);
	 nsf_play_addr = read16_le(buf + 0x04);

	 meta.pal = ((buf[0x06] & 0x3) == 0x1);
	 meta.chip = buf[0x07];
	 meta.song_count = buf[0x08];
	 meta.song_start = buf[0x09];
	}
	break;

   case FOURCC('B', 'A', 'N', 'K'):
	memset(nsf_initial_banks, 0x00, 8);

	if(chunk_size != 8)
	{
	 snprintf(error_message, sizeof(error_message), "NSFe \"%s\" chunk size is wrong.", "BANK");
	 return NULL;
	}

	if(callb->read(callb, nsf_initial_banks, 8) != 8)
	{
	 snprintf(error_message, sizeof(error_message), "Error reading NSFe \"%s\" chunk.", "BANK");
         return NULL;
	}

	nsf_bs_enabled = true;
	break;

   case FOURCC('D', 'A', 'T', 'A'):
	if(nsf_load_addr < 0x8000)
	{
	 snprintf(error_message, sizeof(error_message), "Load address 0x%04X is unsupported.", nsf_load_addr);
	 return NULL;
	}
	//
	{
	 const uint32 rom_offs = (nsf_bs_enabled ? (nsf_load_addr & 0x0FFF) : (nsf_load_addr - 0x8000));

	 if(sizeof(rom) < ((uint64)rom_offs + chunk_size))
	 {
	  snprintf(error_message, sizeof(error_message), "ROM data is too large by %llu bytes.", (unsigned long long)((uint64)rom_offs + chunk_size - sizeof(rom)));
	  return NULL;
	 }

	 if(callb->read(callb, rom + rom_offs, chunk_size) != chunk_size)
	 {
	  snprintf(error_message, sizeof(error_message), "Error reading NSFe \"%s\" chunk.", "DATA");
          return NULL;
	 }
	}
	break;

   case FOURCC('N', 'E', 'N', 'D'):
	goto breakout;
   //
   //
   case FOURCC('a', 'u', 't', 'h'):
	if(!read_nsfe_string(callb, meta.title, sizeof(meta.title)))
	{
	 snprintf(error_message, sizeof(error_message), "Error reading NSFe \"%s\" chunk.", "auth");
	 return NULL;
	}

	if(!read_nsfe_string(callb, meta.artist, sizeof(meta.artist)))
	{
	 snprintf(error_message, sizeof(error_message), "Error reading NSFe \"%s\" chunk.", "auth");
	 return NULL;
	}

	if(!read_nsfe_string(callb, meta.copyright, sizeof(meta.copyright)))
	{
	 snprintf(error_message, sizeof(error_message), "Error reading NSFe \"%s\" chunk.", "auth");
	 return NULL;
	}
	break;

   case FOURCC('t', 'l', 'b', 'l'):
	{
	 unsigned i = 0;

	 while(callb->tell(callb) < (chunk_data_start + chunk_size) && i < 256)
	 {
	  if(!read_nsfe_string(callb, meta.song_titles[i], sizeof(meta.song_titles[i])))
	  {
	   snprintf(error_message, sizeof(error_message), "Error reading NSFe \"%s\" chunk.", "tlbl");
	   return NULL;
	  }
	  i++;
	 }
	}
	break;

   case FOURCC('m', 'i', 'x', 'e'):
	{
	 if(chunk_size % 3)
	 {
	  snprintf(error_message, sizeof(error_message), "NSFe \"%s\" chunk size is wrong.", "mixe");
	  return NULL;
	 }

	 for(uint32 i = 0; i < (chunk_size / 3); i++)
	 {
	  uint8 buf[3];

	  if(callb->read(callb, buf, sizeof(buf)) != sizeof(buf))
	  {
	   snprintf(error_message, sizeof(error_message), "Error reading NSFe \"%s\" chunk.", "mixe");
           return NULL;
	  }
	  //
	  const uint8 device = buf[0];
	  const int16 mb = (int16)read16_le(buf + 1);

	  if(device < (sizeof(nsf_mix_levels) / sizeof(nsf_mix_levels[0])))
	   nsf_mix_levels[device] = mb;
	 }
	}
	break;
  }
  //
  if(callb->seek(callb, chunk_data_start + chunk_size) < 0)
  {
   snprintf(error_message, sizeof(error_message), "File seek failed.");
   return NULL;
  }
 }
 printf("NSFe read end without \"NEND\" chunk!\n");

 breakout:;

 return &meta;
}

static void adjust_mix_levels(void)
{
 //
 // TODO: Relocate max level constants to expansion chip source files as maximum linear ref volumes.
 //
 const int max_mmc5_level = 0;
 const int max_vrc6_level = 0;
 const int max_n163_level = 1204;
 const int max_sun5b_level = -132;
 int32 adj = 0;

 if(nsf_mix_levels[MIX_LEVEL_APU_SQUARE] > 0 && (adj < nsf_mix_levels[MIX_LEVEL_APU_SQUARE]))
  adj = nsf_mix_levels[MIX_LEVEL_APU_SQUARE];

 if((meta.chip_emulated & CHIP_MASK_VRC6) && nsf_mix_levels[MIX_LEVEL_VRC6] > max_vrc6_level && (adj < (nsf_mix_levels[MIX_LEVEL_VRC6] - max_vrc6_level)))
  adj = nsf_mix_levels[MIX_LEVEL_VRC6] - max_vrc6_level;

 if((meta.chip_emulated & CHIP_MASK_MMC5) && nsf_mix_levels[MIX_LEVEL_MMC5] > max_mmc5_level && (adj < (nsf_mix_levels[MIX_LEVEL_MMC5] - max_mmc5_level)))
  adj = nsf_mix_levels[MIX_LEVEL_MMC5] - max_mmc5_level;

 if((meta.chip_emulated & CHIP_MASK_N163) && nsf_mix_levels[MIX_LEVEL_N163] > max_n163_level && (adj < (nsf_mix_levels[MIX_LEVEL_N163] - max_n163_level)))
  adj = nsf_mix_levels[MIX_LEVEL_N163] - max_n163_level;

 if((meta.chip_emulated & CHIP_MASK_SUN5B) && nsf_mix_levels[MIX_LEVEL_SUN5B] > max_sun5b_level && (adj < (nsf_mix_levels[MIX_LEVEL_SUN5B] - max_sun5b_level)))
  adj = nsf_mix_levels[MIX_LEVEL_SUN5B] - max_sun5b_level;

 if(adj > 0)
 {
  for(unsigned i = 0; i < (sizeof(nsf_mix_levels) / sizeof(nsf_mix_levels[0])); i++)
  {
   nsf_mix_levels[i] -= adj;
  }
 }

#if 0
 for(unsigned i = 0; i < (sizeof(nsf_mix_levels) / sizeof(nsf_mix_levels[0])); i++)
 {
  printf("%d: % 6d\n", i, nsf_mix_levels[i]);
 }
#endif
}

COLD_SECTION static const nsf_meta_t* nsf_load(nsfcore_file_callb_t* callb)
{
 uint16 nsf_load_addr = 0;
 uint8 header[0x80];

 if(callb->read(callb, header, 5) != 5)
 {
  snprintf(error_message, sizeof(error_message), "Error reading NSF header.");
  return NULL;
 }

 if(header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 'M' || header[4] != 0x1A)
 {
  snprintf(error_message, sizeof(error_message), "NSF file header magic string not present.");
  return NULL;
 }

 if(callb->read(callb, header + 5, sizeof(header) - 5) != (sizeof(header) - 5))
 {
  snprintf(error_message, sizeof(error_message), "Error reading NSF header.");
  return NULL;
 }
 //
 //
 meta.song_count = header[0x06];
 meta.song_start = header[0x07] ? header[0x07] - 1 : 0;

 nsf_load_addr = read16_le(header + 0x08);
 nsf_init_addr = read16_le(header + 0x0A);
 nsf_play_addr = read16_le(header + 0x0C);

 memcpy(meta.title, header + 0x0E, 32);
 meta.title[32] = 0;

 memcpy(meta.artist, header + 0x2E, 32);
 meta.artist[32] = 0;

 memcpy(meta.copyright, header + 0x4E, 32);
 meta.copyright[32] = 0;

 for(unsigned i = 0; i < 32; i++)
 {
  meta.title[i] = iso88591_to_cp850[(unsigned char)meta.title[i]];
  meta.artist[i] = iso88591_to_cp850[(unsigned char)meta.artist[i]];
  meta.copyright[i] = iso88591_to_cp850[(unsigned char)meta.copyright[i]];
 }

 meta.pal = ((header[0x7A] & 0x3) == 0x1);
 meta.chip = header[0x7B];

 memcpy(nsf_initial_banks, header + 0x70, 8);

 nsf_bs_enabled = (bool)read64_be(nsf_initial_banks);

 if(nsf_load_addr < 0x8000)
 {
  snprintf(error_message, sizeof(error_message), "Load address 0x%04X is unsupported.", nsf_load_addr);
  return NULL;
 }

 const uint32 rom_offs = (nsf_bs_enabled ? (nsf_load_addr & 0x0FFF) : (nsf_load_addr - 0x8000));

 if(callb->read(callb, rom + rom_offs, sizeof(rom) - rom_offs) < 0)
 {
  snprintf(error_message, sizeof(error_message), "Error reading NSF ROM data.");
  return NULL;
 }

 {
  unsigned char tmp;

  if(callb->read(callb, &tmp, 1) == 1)
  {
   snprintf(error_message, sizeof(error_message), "ROM data is too large.");
   return NULL;
  }
 }

 return &meta;
}

const nsf_meta_t* nsfcore_load(nsfcore_file_callb_t* callb)
{
 const nsf_meta_t* ret = NULL;
 uint8 magic[4];

 nsfcore_close();
 //
 //
 memset(nsf_mix_levels, 0, sizeof(nsf_mix_levels));

 nsf_mix_levels[MIX_LEVEL_APU_SQUARE] = 0;
 nsf_mix_levels[MIX_LEVEL_VRC6] = 0;
 nsf_mix_levels[MIX_LEVEL_N163] = 1204;
 nsf_mix_levels[MIX_LEVEL_FDS] = 700;
 nsf_mix_levels[MIX_LEVEL_MMC5] = 0;
 nsf_mix_levels[MIX_LEVEL_SUN5B] = -130;

 if(callb->seek(callb, 0) < 0)
 {
  snprintf(error_message, sizeof(error_message), "File seek failed.");
  return NULL;
 }

 if(callb->read(callb, magic, 4) != 4)
 {
  snprintf(error_message, sizeof(error_message), "Error reading file header.");
  return NULL;
 }

 if(callb->seek(callb, 0) < 0)
 {
  snprintf(error_message, sizeof(error_message), "File seek failed.");
  return NULL;
 }

 if(!memcmp(magic, "NSFE", 4))
  ret = nsfe_load(callb);
 else
  ret = nsf_load(callb);
 //
 //
 //
 if(meta.chip & CHIP_MASK_VRC6)
  meta.chip_emulated = CHIP_MASK_VRC6;
 else if(meta.chip & CHIP_MASK_N163)
  meta.chip_emulated = CHIP_MASK_N163;
 else if(meta.chip & CHIP_MASK_SUN5B)
  meta.chip_emulated = CHIP_MASK_SUN5B;
 else if(meta.chip & CHIP_MASK_MMC5)
  meta.chip_emulated = CHIP_MASK_MMC5;
 //
 //
 //
 adjust_mix_levels();

 return ret;
}

#define SET_READ(addr, p)			\
{						\
 intptr_t d = (intptr_t)(p) - (intptr_t)ram;	\
 assert(!(d & 0x3));				\
 assert(d >= -128 * 4 && d < 0);		\
 rfap[addr] = d >> 2;				\
}

#define SET_WRITE(addr, p)			\
{						\
 wfap[addr] = (uintptr_t)(p);			\
}

static void slave_write(uint16 timestamp, uint8 A, uint8 V)
{
 *(volatile uint32*)((volatile uint8*)exchip_rb + exchip_rb_wr) = (timestamp << 16) | (A << 8) | V;
 exchip_rb_wr += 4;
}

enum
{
 SLAVECMD_FORCE_UPDATE = 0xFD,
 SLAVECMD_FRAME = 0xFE,
 SLAVECMD_STOP = 0xFF
};

LORAM_BSS_UNCACHED volatile int16 nsfcore_slave_ready;
static LORAM_BSS_UNCACHED volatile int16 run_slave;

static void slave_stop(void)
{
 if(!meta.chip_emulated)
  return;
 //
 //
 if(run_slave)
 {
  run_slave = false;
  //
  //
  //
  while(nsfcore_slave_ready)
  {
   slave_write(0, SLAVECMD_STOP, 0);
   *(volatile uint16*)0x21000000 = 0;

   sh2_wait_approx(1000);
  }
 }
}

static void slave_start(void)
{
 if(!meta.chip_emulated)
  return;
 //
 //
 if(!run_slave)
 {
  exchip_rb_wr = 0;
  memset((void*)exchip_rb, 0, sizeof(exchip_rb));
  //
  run_slave = true;
  *(volatile uint16*)0x21000000 = 0;
  while(!nsfcore_slave_ready) { sh2_wait_approx(1000); }
 }
}

static void slave_force_update(uint32 timestamp)
{
 if(!meta.chip_emulated)
  return;
 //
 //
 slave_write(timestamp, SLAVECMD_FORCE_UPDATE, 0);
 //
 *(volatile uint16*)0x21000000 = 0;
}

static void slave_frame(uint32 timestamp)
{
 if(!meta.chip_emulated)
  return;
 //
 //
 slave_write(timestamp, SLAVECMD_FRAME, 0);
 //
 *(volatile uint16*)0x21000000 = 0;
}

uint32 exp2_fxp_16_16(const int32 twex)
{
 static const uint16 exptab[512] =
 {
  #include "exptab.h"
 };
 const size_t index = twex & 0xFFFF;
 const int shift = (twex >> 16);
 unsigned a;
 unsigned b;

 if(shift < -31)
  return 0;

 if(shift >= 0)
 {
  a = ((65536 + (uint32)exptab[(index >> 7) + 0]) << shift);
  b = (((index >> 7) != 511) ? (((65536 + exptab[(index >> 7) + 1]))) : 131072) << shift;
 }
 else
 {
  a = ((65536 + (uint32)exptab[(index >> 7) + 0]) >> -shift);
  b = (((index >> 7) != 511) ? (((65536 + exptab[(index >> 7) + 1]))) : 131072) >> -shift;
 }

 return a + (((b - a) * (index & 0x7F)) >> 7);
}

static uint32 mb_to_linear_16_16(const int16 mb)
{
 return exp2_fxp_16_16((mb * (long long)7133786) >> 16);
}

COLD_SECTION static void emu_init(void)
{
 slave_stop();
 //printf("Song: %u\n", nsf_song_number);
 //printf("Init: 0x%04x\n", nsf_init_addr);
 //printf("Play: 0x%04x\n", nsf_play_addr);

 frame_alt = 0;
 time_fract = 0;
 timestamp_base = 0;

 nsf_rom_base = rom;
 nsf_frame_pending = 0x00;

 shim_6502[read16_le(shim_6502 + 0xFF2) & 0xFFF] = nsf_song_number;
 shim_6502[read16_le(shim_6502 + 0xFF4) & 0xFFF] = meta.pal; //nsf_pal;
 write16_le(shim_6502 + (read16_le(shim_6502 + 0xFF6) & 0xFFF), nsf_init_addr);
 write16_le(shim_6502 + (read16_le(shim_6502 + 0xFF8) & 0xFFF), nsf_play_addr);
 set_bank_32k(0x8, rom);

 if(nsf_bs_enabled)
 {
  for(unsigned i = 0; i < 8; i++)
  {
   set_bank_4k(0x8 + i, nsf_rom_base + (nsf_initial_banks[i] << 12)); // FIXME: non-power-of-2 max ROM data size
  }
 }

 for(uint16 addr = 0x5FF8; addr < 0x6000; addr++)
 {
  SET_WRITE(addr, nsf_bs_enabled ? nsf_set_bank_S : ob_rw_func_S);
 }

 if(meta.chip_emulated)
 {
  uint32 exchip_mix_level = 0;

  if(meta.chip_emulated & CHIP_MASK_VRC6)
  {
   exchip_slave_entry = vrc6_slave_entry;
   exchip_mix_level = nsf_mix_levels[MIX_LEVEL_VRC6];

   for(unsigned addr = 0x9000; addr < 0xC000; addr++)
   {
    SET_WRITE(addr, vrc6_write_func_S);
   }
  }
  else if(meta.chip_emulated & CHIP_MASK_N163)
  {
   exchip_slave_entry = n163_slave_entry;
   exchip_mix_level = nsf_mix_levels[MIX_LEVEL_N163];

   for(unsigned addr = 0x4800; addr < 0x5000; addr++)
   {
    SET_WRITE(addr, n163_write_4800_func_S);
    SET_READ(addr, n163_read_4800_func_S);
   }

   for(unsigned addr = 0xF800; addr < 0x10000; addr++)
   {
    SET_WRITE(addr, n163_write_f800_func_S);
   }
  }
  else if(meta.chip_emulated & CHIP_MASK_SUN5B)
  {
   exchip_slave_entry = sun5b_slave_entry;
   exchip_mix_level = nsf_mix_levels[MIX_LEVEL_SUN5B];

   for(unsigned addr = 0xC000; addr < 0xE000; addr++)
   {
    SET_WRITE(addr, sun5b_write0_func_S);
   }

   for(unsigned addr = 0xE000; addr < 0x10000; addr++)
   {
    SET_WRITE(addr, sun5b_write1_func_S);
   }
  }
  else if(meta.chip_emulated & CHIP_MASK_MMC5)
  {
   exchip_slave_entry = mmc5_slave_entry;
   exchip_mix_level = nsf_mix_levels[MIX_LEVEL_MMC5];

   SET_WRITE(0x5000, mmc5_write0_func_S);
   SET_WRITE(0x5003, mmc5_write3_func_S);
   SET_WRITE(0x5004, mmc5_write4_func_S);
   SET_WRITE(0x5007, mmc5_write7_func_S);

   SET_WRITE(0x5002, mmc5_writeX_func_S);
   SET_WRITE(0x5006, mmc5_writeX_func_S);

   SET_WRITE(0x5010, mmc5_writeX_func_S);
   SET_WRITE(0x5011, mmc5_writeX_func_S);

   SET_WRITE(0x5015, mmc5_write_status_func_S);
   SET_READ(0x5015, mmc5_read_status_func_S);

   SET_WRITE(0x5205, mmc5_write_mult0_func_S);
   SET_WRITE(0x5206, mmc5_write_mult1_func_S);

   SET_READ(0x5205, mmc5_read_mult_res0_func_S);
   SET_READ(0x5206, mmc5_read_mult_res1_func_S);
  }
  //
  exchip_volume = ((int64)106 * 5424 * mb_to_linear_16_16(exchip_mix_level)) >> (16 + 7);
 }
 //
 //
 //
 apu_init(meta.pal, mb_to_linear_16_16(nsf_mix_levels[MIX_LEVEL_APU_SQUARE]));

 apu_power();
 s6502_power();

 if(meta.chip_emulated & CHIP_MASK_MMC5)
  mmc5_master_power();

 memset(ram, 0x00, sizeof(ram));
 memset(prgram, 0x00, sizeof(prgram));

 {
  uint8* p = (uint8*)(banks_4k[0xF] + 0xF000);
  const uint16 save = read16_le(p + 0xFFC);

  write16_le(p + 0xFFC, read16_le(shim_6502 + 0xFF0));

  s6502_reset();

  write16_le(p + 0xFFC, save);
 }
 //
 //
 //
 slave_start();
 //
 //
 //
 apu_start_sync();

 timestamp_base = 0;
}

void nsfcore_run_frame(void)
{
 unsigned torun;

 nsf_frame_pending |= 0x01;
 frame_alt = !frame_alt;

 for(unsigned i = 0; i < 2; i++)
 {
  if(meta.pal)
  {
   time_fract += 341 * 5 * 156;
   torun = time_fract >> 4;
   time_fract &= 0xF;
  }
  else
  {
   time_fract += 341 * 131 - (frame_alt & !i);
   torun = time_fract / 3;
   time_fract %= 3;
  }
  //
  //
  timestamp_base += torun;
  s6502_run(torun);
  //
  //
  const uint32 timestamp = timestamp_base + s6502.CYC_CNT;

  if(!i)
  {
   apu_force_update(timestamp);
   //
   slave_force_update(timestamp);
  }
  else
  {
   // Call slave_frame() before apu_frame(), which blocks.
   slave_frame(timestamp);
   //
   //
   if(meta.chip_emulated & CHIP_MASK_MMC5)
    mmc5_master_frame(timestamp);
   //
   //
   apu_frame(timestamp);
   assert(s6502.CYC_CNT >= 0);
   //
   //
   timestamp_base -= timestamp;
  }
 }

#if EXCHIP_BENCH
 if(meta.chip_emulated)
 {
  const uint32 ebrbwr = exchip_bench_rb_wr;

  while(exchip_bench_rb_rd != ebrbwr)
  {
   uint32 eb = exchip_bench[exchip_bench_rb_rd];
   char tmp[11];
   char* p = tmp + sizeof(tmp);

   exchip_bench_rb_rd = (exchip_bench_rb_rd + 1) & 7;

   *(--p) = '\n';

   while(eb)
   {
    *(--p) = '0' + (eb % 10);
    eb /= 10;
   }

   write(1, p, sizeof(tmp) - (p - tmp));
  }
 }
#endif
}

void nsfcore_set_song(uint8 w)
{
 nsf_song_number = w;

 emu_init();
}

bool nsfcore_special(uint16 jsr_addr, uint8 accum_param)
{
 if(nsf_frame_pending & 0x80)
  return false;
 //
 //
 shim_6502[read16_le(shim_6502 + 0xFFC) & 0xFFF] = accum_param;
 write16_le(shim_6502 + (read16_le(shim_6502 + 0xFFE) & 0xFFF), jsr_addr);

 nsf_frame_pending |= 0x80;

 return true;
}

COLD_SECTION bool nsfcore_load_scsp_bin(nsfcore_file_callb_t* callb)
{
 if(callb->read(callb, (void*)SCSPVP(0), 0x80000) != 0x80000)
 {
  snprintf(error_message, sizeof(error_message), "Error reading SCSP RAM data stream.");
  return false;
 }

 return true;
}

COLD_SECTION bool nsfcore_load_exchip_bin(nsfcore_file_callb_t* callb)
{
 if(callb->size(callb) != sizeof(exchip_tables))
 {
  snprintf(error_message, sizeof(error_message), "File size is wrong!");
  return false;
 }

 if(callb->read(callb, &exchip_tables, sizeof(exchip_tables)) != sizeof(exchip_tables))
 {
  snprintf(error_message, sizeof(error_message), "Error reading expansion chip data stream.");
  return false;
 }

 return true;
}

COLD_SECTION bool nsfcore_swapload_scsp_bin(volatile void* mem)
{
 volatile uint32_MA* s = (volatile uint32_MA*)SCSPVP(0);
 volatile uint32_MA* m = (volatile uint32_MA*)mem;
 //
 const uint32 scsp_sig = s[0x1FFFF];
 const uint32 mem_sig = m[0x1FFFF];
 const uint32 expected_sig = meta.pal ? FOURCC('P', 'A', 'L', ' ') : FOURCC('N', 'T', 'S', 'C');

 if(scsp_sig != expected_sig)
 {
  if(mem_sig != expected_sig)
  {
   snprintf(error_message, sizeof(error_message), "Error in nsfcore_swapload_scsp_bin().");
   return false;
  }
  //
  //
  for(unsigned i = 0x20000; i; i--)
  {
   uint32 tmp = *s;

   *s = *m;
   s++;
   *m = tmp;
   m++;
  }
 }

 return true;
}

void nsfcore_slave_entry(void)
{
 // FIXME, potential race.
 TIER = 0;

 for(;;)
 {
  while(!run_slave)
  {
   while(!(FTCSR & 0x80))
   {
    sh2_wait_approx(1000);
   }
   FTCSR = 0;
  }
  //
  //
  // nsfcore_slave_ready is set to 'true' in vrc6_slave_entry()
  {
   uint32 scale = apu_get_timestamp_scale();
   volatile int16* buf = apu_get_exchip_buffer();

   CCR = CCR_CE | CCR_TW | CCR_CP;
   CCR;
   asm volatile(
	"mov.l r15, @-%[stack_top]\n\t"
	"mov %[stack_top], r15\n\t"
	"mov %[scale], r4\n\t"
	"mov %[buf], r5\n\t"

	"jsr @%[exchip_slave_entry]\n\t"
	"mov %[exchip_volume], r6\n\t"

	"mov.l @%[stack_top]+, r15\n\t"
	:
	: [stack_top] "r"(0xC0000000 + 2048), [exchip_slave_entry] "r"(exchip_slave_entry), [scale] "r"(scale), [buf] "r"(buf),
	  [exchip_volume] "r"(exchip_volume)
	: "cc", "memory", "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r14", "pr");
   //
   CCR = CCR_CE | CCR_CP;
   CCR;
  }
  //
  nsfcore_slave_ready = false;
 }
}

bool nsfcore_init(void)
{
 assert(!((uintptr_t)exchip_rb & 0xFFFF));

 apu_preinit();
 //
 //
 //
 s6502_init();

 for(unsigned addr = 0x0000; addr < 0x10000; addr++)
 {
  SET_READ(addr, ob_rw_func_S);
  SET_WRITE(addr, ob_rw_func_S);
 }

 for(unsigned addr = 0x4000; addr < 0x4018; addr++)
 {
  if(addr != 0x4009 && addr != 0x400D && addr != 0x4014 && addr != 0x4016)
  {
   SET_WRITE(addr, s6502_write_shim_S);
  }
 }
/*
 SET_WRITE(0x4012, apu_write_4012_S);
 SET_WRITE(0x4013, apu_write_4013_S);
*/
 SET_READ(0x4015, apu_read_4015_S);

 for(unsigned addr = 0x0000; addr < 0x0800; addr++)
 {
  SET_READ(addr, ram_read_func_S);
  SET_WRITE(addr, ram_write_func_S);
 }

 for(unsigned addr = 0x0800; addr < 0x2000; addr++)
 {
  SET_READ(addr, ram_mirror_read_func_S);
  SET_WRITE(addr, ram_mirror_write_func_S);
 }

 for(unsigned addr = 0x6000; addr < 0x7000; addr++)
 {
  SET_READ(addr, bs6xxx_read_func_S);
  SET_WRITE(addr, bs6xxx_write_func_S);
 }

 for(unsigned addr = 0x7000; addr < 0x8000; addr++)
 {
  SET_READ(addr, bs7xxx_read_func_S);
  SET_WRITE(addr, bs7xxx_write_func_S);
 }

 for(unsigned addr = 0x8000; addr < 0x9000; addr++)
  SET_READ(addr, bs8xxx_read_func_S);

 for(unsigned addr = 0x9000; addr < 0xA000; addr++)
  SET_READ(addr, bs9xxx_read_func_S);

 for(unsigned addr = 0xA000; addr < 0xB000; addr++)
  SET_READ(addr, bsAxxx_read_func_S);

 for(unsigned addr = 0xB000; addr < 0xC000; addr++)
  SET_READ(addr, bsBxxx_read_func_S);

 for(unsigned addr = 0xC000; addr < 0xD000; addr++)
  SET_READ(addr, bsCxxx_read_func_S);

 for(unsigned addr = 0xD000; addr < 0xE000; addr++)
  SET_READ(addr, bsDxxx_read_func_S);

 for(unsigned addr = 0xE000; addr < 0xF000; addr++)
  SET_READ(addr, bsExxx_read_func_S);

 for(unsigned addr = 0xF000; addr < 0x10000; addr++)
  SET_READ(addr, bsFxxx_read_func_S);
 //
 //
 set_bank_4k(0x0, ram);
 set_bank_4k(0x1, ram);

 set_bank_8k(0x6, prgram);

 set_bank_4k(0x2, shim_6502);
 for(unsigned addr = 0x2000; addr < 0x3000; addr++)
  SET_READ(addr, bs2xxx_read_func_S);

 SET_READ(0x3EFF, nsf_read_frame_pending_S);

 return true;
}

COLD_SECTION static ssize_t nsfcore_mem_read(struct nsfcore_file_callb_t_* callb, void* ptr, size_t count)
{
 const uint8* data = callb->priv;
 const int32 size = callb->priv_u32[0];
 int32 pos = callb->priv_u32[1];

 if(pos >= size)
  return 0;

 if(count > (size - pos))
  count = size - pos;

 memcpy(ptr, data + pos, count);

 pos += count;

 callb->priv_u32[1] = pos;

 return count;
}

COLD_SECTION static int32 nsfcore_mem_tell(struct nsfcore_file_callb_t_* callb)
{
 return callb->priv_u32[1];
}

COLD_SECTION static int nsfcore_mem_seek(struct nsfcore_file_callb_t_* callb, int32 new_position)
{
 if(new_position < 0)
  return -1;

 callb->priv_u32[1] = new_position;

 return 0;
}

COLD_SECTION static int32 nsfcore_mem_size(struct nsfcore_file_callb_t_* callb)
{
 return callb->priv_u32[0];
}

void nsfcore_make_mem_callb(nsfcore_file_callb_t* callb, const void* data, uint32 size)
{
 memset(callb, 0, sizeof(*callb));

 callb->read = nsfcore_mem_read;
 callb->seek = nsfcore_mem_seek;
 callb->tell = nsfcore_mem_tell;
 callb->size = nsfcore_mem_size;

 callb->priv = (void*)data;
 callb->priv_u32[0] = size;
 callb->priv_u32[1] = 0;
}

static ssize_t nsfcore_stdio_read(struct nsfcore_file_callb_t_* callb, void* ptr, size_t count)
{
 FILE* stream = (FILE*)callb->priv;
 size_t rv = fread(stream, 1, count, ptr);

 if(ferror(stream))
 {
  clearerr(stream);
  return -1;
 }

 return rv;
}

static int32 nsfcore_stdio_tell(struct nsfcore_file_callb_t_* callb)
{
 FILE* stream = (FILE*)callb->priv;
 int32 ret = ftell(stream);

 clearerr(stream);

 return ret;
}

static int nsfcore_stdio_seek(struct nsfcore_file_callb_t_* callb, int32 new_position)
{
 FILE* stream = (FILE*)callb->priv;
 int ret = fseek(stream, new_position, SEEK_SET);

 clearerr(stream);

 return ret;
}

static int32 nsfcore_stdio_size(struct nsfcore_file_callb_t_* callb)
{
 FILE* stream = (FILE*)callb->priv;
 const int32 rpos = ftell(stream);
 int32 ret;

 if(rpos == -1)
  return -1;

 if(fseek(stream, 0, SEEK_END) < 0)
  return -1;

 ret = ftell(stream);
 if(fseek(stream, rpos, SEEK_SET) < 0 || ret == -1)
  return -1;

 return ret;
}

void nsfcore_make_stdio_callb(nsfcore_file_callb_t* callb, FILE* stream)
{
 memset(callb, 0, sizeof(*callb));

 callb->read = nsfcore_stdio_read;
 callb->seek = nsfcore_stdio_seek;
 callb->tell = nsfcore_stdio_tell;
 callb->size = nsfcore_stdio_size;

 callb->priv = (void*)stream;
}

void nsfcore_close(void)
{
 slave_stop();
 //
 apu_kill();
 //
 memset(&meta, 0, sizeof(meta));
 memset(rom, 0, sizeof(rom));

 memset(nsf_initial_banks, 0x00, 8);
 nsf_play_addr = 0;
 nsf_init_addr = 0;
 nsf_bs_enabled = false;
}

void nsfcore_kill(void)
{
 nsfcore_close();
}

