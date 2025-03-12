/*
 * cronsf.c
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

#include "gs/all.h"

#include "version.h"
#include "nsfcore.h"
#include "gfx.h"
#include "fsys.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

enum
{
 PHASE_FILE_SELECTOR = 0,
 PHASE_TEXT_VIEWER,
 PHASE_HEX_VIEWER,
 PHASE_PLAYER,
 PHASE_INIT
};

static unsigned phase = PHASE_FILE_SELECTOR;

enum
{
 FILE_TYPE_NSF = FILE_TYPE__USER,
 FILE_TYPE_NSFE,
 FILE_TYPE_NES,
 FILE_TYPE_TXT,
};

static int32 filesel_scroll = 0;
static int32 filesel_which = 0;

typedef struct filesel_stack_t_
{
 int32 filesel_which;
 int32 filesel_scroll;
 uint32 dir_lba;
 uint32 dir_size;
} filesel_stack_t;

static filesel_stack_t filesel_stack[128];
static uint32 filesel_stack_offs;
//
//
static char* text_buffer;
static size_t text_buffer_size;

static uint32 text_num_lines;
static uint32 text_scroll;
//
//
static uint8* hex_buffer;
static uint32 hex_buffer_size;
static uint32 hex_size;
static uint32 hex_num_lines;
static uint32 hex_scroll;
//
//
VDP1_BSS_UNCACHED static uint8 scsp_ram_pal_init[524288];

static const nsf_meta_t* nsf_meta;
static uint8 nsf_song_number;

static bool hires_mode;
static bool hires_mode_save;	// For hex viewer.

static uint16 buttons;
static uint16 buttons_prev;
static uint16 buttons_np;
static uint16 buttons_held[16];

enum
{
 BUTTON_B = 0,
 BUTTON_C = 1,
 BUTTON_A = 2,
 BUTTON_START = 3,
 BUTTON_UP = 4,
 BUTTON_DOWN = 5,
 BUTTON_LEFT = 6,
 BUTTON_RIGHT = 7,
 BUTTON_L = 11,
 BUTTON_Z = 12,
 BUTTON_Y = 13,
 BUTTON_X = 14,
 BUTTON_R = 15,
};

enum
{
 BUTTON_MASK_B		= 1U << BUTTON_B,
 BUTTON_MASK_C		= 1U << BUTTON_C,
 BUTTON_MASK_A		= 1U << BUTTON_A,
 BUTTON_MASK_START	= 1U << BUTTON_START,
 BUTTON_MASK_UP		= 1U << BUTTON_UP,
 BUTTON_MASK_DOWN	= 1U << BUTTON_DOWN,
 BUTTON_MASK_LEFT	= 1U << BUTTON_LEFT,
 BUTTON_MASK_RIGHT	= 1U << BUTTON_RIGHT,
 BUTTON_MASK_L		= 1U << BUTTON_L,
 BUTTON_MASK_Z		= 1U << BUTTON_Z,
 BUTTON_MASK_Y		= 1U << BUTTON_Y,
 BUTTON_MASK_X		= 1U << BUTTON_X,
 BUTTON_MASK_R		= 1U << BUTTON_R
};

static volatile uint32 slave_buttons;

//
// Branched to from _slave_vblank_interrupt_S in cronsf_s.S:
//
__attribute__((interrupt_handler)) void slave_vblank_interrupt(void)
{
 if(!(SMPC_SF & 0x1))
 {
  if(SMPC_OREG(0) & 0xF)
  {
   if((SMPC_OREG(1) & 0xF0) < 0xE0)
   {
    uint16 nb;

    nb  = SMPC_OREG(2) << 0;
    nb |= SMPC_OREG(3) << 8;
    slave_buttons = (uint16)~nb;
   }
  }
  //
  //
  SMPC_SF = 1;

  if(SMPC_SR & 0x10)
  {
   SMPC_COMREG = SMPC_CMD_SYSRES;
   for(;;) ;
  }
  else
  {
   SMPC_IREG(0) = 0x00;
   SMPC_IREG(1) = 0xCA;
   SMPC_IREG(2) = 0xF0;

   SMPC_COMREG = SMPC_CMD_INTBACK;
  }
 }
 //
 //
 //
 WTCSR_R;
 WRITE_WTCSR(0x25);
}

void slave_wdt_interrupt_S(void);
void slave_vblank_interrupt_S(void);

static void slave_entry(void)
{
 static uint32 slave_vbr[0x80];

 for(unsigned i = 0; i < 0x10; i++)
  slave_vbr[0x40 + i] = (uint32)slave_vblank_interrupt_S;

 sh2_set_sr(0xF0);
 sh2_set_vbr((uint32)slave_vbr);
 //
 //
 WTCSR_R;
 WRITE_WTCSR(0x00);

 IPRA = 0x00F0;
 VCRWDT = 0x7F00;
 slave_vbr[0x7F] = (uint32)slave_wdt_interrupt_S;
 //
 //
 sh2_set_sr(0x50);
 //
 nsfcore_slave_entry();
}

static void init_input(void)
{
 SMPC_PDR1 = 0;
 SMPC_PDR2 = 0;

 SMPC_DDR1 = 0;
 SMPC_DDR2 = 0;

 SMPC_IOSEL = 0;
 SMPC_EXLE = 0;

 buttons = buttons_prev = 0xFFFF;
 buttons_np = 0;
 memset(buttons_held, 0, sizeof(buttons_held));

 slave_buttons = (uint32)-1;
 //
 //
 //
 smpc_ssh_off();

 bios_slave_entry = slave_entry;

 smpc_ssh_on();
}

static void update_input(void)
{
 const uint32 sbc = *(volatile uint32*)((uintptr_t)&slave_buttons | 0x20000000);

 if(sbc == (uint32)-1)
  return;
 //
 //
 buttons_prev = buttons;
 buttons = sbc;
 buttons_np = (buttons ^ buttons_prev) & buttons;

 for(unsigned i = 0; i < 16; i++)
 {
  if((buttons_np >> i) & 1)
   buttons_held[i] = 1;
  else if((buttons >> i) & 1)
   buttons_held[i] += (buttons_held[i] != (uint16)-1 && buttons_held[i] != 0);
  else
   buttons_held[i] = 0;
 }
}

static void player_draw_(bool hires_draw)
{
 const unsigned w = hires_draw ? 80 : 40;

 gfx_draw_box(0, 2, w, 3, "Title");
 gfx_draw_box(0, 5, w, 3, "Artist");
 gfx_draw_box(0, 8, w, 3, "Copyright");
 gfx_draw_box(0, 11, w, 3, "Song");

 const unsigned text_color = 0x7;

 gfx_draw_text_trunc(2, 2 + 1, w - 4, nsf_meta->title, text_color, 0x09);
 gfx_draw_text_trunc(2, 5 + 1, w - 4, nsf_meta->artist, text_color, 0x09);
 gfx_draw_text_trunc(2, 8 + 1, w - 4, nsf_meta->copyright, text_color, 0x09);

 gfx_draw_text_trunc(2, 11 + 1, w - 4, nsf_meta->song_titles[nsf_song_number], text_color, 0x09);

 char tmp[11];

 snprintf(tmp, sizeof(tmp), "%03u", nsf_song_number + 1);
 gfx_draw_char(w / 2 - strlen(tmp) - 1, 11, 0xAE, 0x08);
 gfx_draw_text(w / 2 - strlen(tmp), 11, 3, tmp, 0x02);
 gfx_draw_char(w / 2, 11, '/', 0x07);

 snprintf(tmp, sizeof(tmp), "%03u", nsf_meta->song_count);
 gfx_draw_text(w / 2 + 1, 11, 3, tmp, 0x02);
 gfx_draw_char(w / 2 + 1 + strlen(tmp), 11, 0xAF, 0x08);

 {
  const int xb = w / 2 - 7;

  gfx_draw_char(xb + 12, 1, 0x0E, 0x0D);

  gfx_draw_char(xb + 0, 1, 'C', 0x0C);
  gfx_draw_char(xb + 2, 1, 'r', 0x06);
  gfx_draw_char(xb + 4, 1, 'o', 0x0E);
  gfx_draw_char(xb + 6, 1, 'N', 0x0A);
  gfx_draw_char(xb + 8, 1, 'S', 0x0B);
  gfx_draw_char(xb + 10, 1, 'F', 0x09);
 }

 if(nsf_meta->chip_emulated != nsf_meta->chip)
 {
  gfx_draw_text((w / 2 - 9), 13, w - (w / 2 - 9), " Unsupported chip(s)! ", 0x40);
 }
}

static void player_draw(void)
{
 gfx_begin_draw(false);
 player_draw_(false);

 gfx_set_draw(true);
 player_draw_(true);
 //
 gfx_update_ntptr();
}

static bool filter(const char* file_name, uint16* file_type)
{
 const char *dp = strrchr(file_name, '.');

 if(dp)
 {
  static const struct
  {
   const char* ext;
   uint16 file_type;
  } extft[] =
  {
   { ".nsf", FILE_TYPE_NSF },
   { ".nsfe", FILE_TYPE_NSFE },
   { ".nes", FILE_TYPE_NES },
   { ".txt", FILE_TYPE_TXT },
   { ".nfo", FILE_TYPE_TXT },
   { ".diz", FILE_TYPE_TXT },
  };

  for(unsigned efi = 0; efi < (sizeof(extft) / sizeof(extft[0])); efi++)
  {
   if(!strcasecmp(dp, extft[efi].ext))
   {
    *file_type = extft[efi].file_type;
    break;
   }
  }
 }

 return true;
}

static bool check_held(unsigned w)
{
 return (buttons_held[w] == 1 || (buttons_held[w] >= 32 && !(buttons_held[w] & 0x03)));
}

static bool run_player(void)
{
 const uint16 np = (buttons ^ buttons_prev) & buttons;

 if(buttons_held[BUTTON_START] == 2 || buttons_held[BUTTON_B] == 24)
 {
  nsfcore_close();
  return true;
 }

 {
  int delta = 0;
  int new_nsn;

  delta += check_held(BUTTON_RIGHT);
  delta -= check_held(BUTTON_LEFT);
  delta += 10 * check_held(BUTTON_DOWN);
  delta -= 10 * check_held(BUTTON_UP);
  delta += 50 * check_held(BUTTON_Y);
  delta -= 50 * check_held(BUTTON_X);


  if(delta | (np & (BUTTON_MASK_A | BUTTON_MASK_C)))
  {
   new_nsn = nsf_song_number + delta;

   if((buttons_np == BUTTON_MASK_LEFT || buttons_np == BUTTON_MASK_RIGHT) && buttons == buttons_np)
   {
    if(new_nsn < 0)
     new_nsn = nsf_meta->song_count - 1;
    else if(new_nsn >= nsf_meta->song_count)
     new_nsn = 0;
   }
   else
   {
    if(new_nsn < 0)
     new_nsn = 0;
    else if(new_nsn >= nsf_meta->song_count)
     new_nsn = nsf_meta->song_count - 1;
   }

   nsf_song_number = new_nsn;
   //
   //
   player_draw();
   //
   // Call nsfcore_set_song() near end, player_draw() takes too much
   // CPU time and will break stuff if it's called after.
   nsfcore_set_song(nsf_song_number);
  }
 }

 nsfcore_run_frame();

 return false;
}

static char error_message[128 + 1];

static bool pop_filesel(void)
{
 if(filesel_stack_offs > 0)
 {
  filesel_stack_t* fe = &filesel_stack[filesel_stack_offs - 1];

  if(!fsys_restore_cur_dir(fe->dir_lba, fe->dir_size, filter))
   snprintf(error_message, sizeof(error_message), "%s", fsys_get_error());
  else
  {
   filesel_stack_offs--;
   //
   filesel_which = fe->filesel_which;
   filesel_scroll = fe->filesel_scroll;

   return true;
  }
 }

 return false;
}

static bool push_filesel(void)
{
 if(filesel_stack_offs < (sizeof(filesel_stack) / sizeof(filesel_stack[0])))
 {
  filesel_stack_t* fe = &filesel_stack[filesel_stack_offs];

  if(fsys_save_cur_dir(&fe->dir_lba, &fe->dir_size))
  {
   fe->filesel_which = filesel_which;
   fe->filesel_scroll = filesel_scroll;

   filesel_stack_offs++;

   return true;
  }
 }

 return false;
}

static fsys_dir_entry_t* run_file_selector(void)
{
 if(buttons_held[BUTTON_B] == 2)
  pop_filesel();
 else if(fsys_num_dir_entries())
 {
  if(buttons_held[BUTTON_START] == 2 || buttons_held[BUTTON_A] == 2 || buttons_held[BUTTON_C] == 2)
  {
   fsys_dir_entry_t* de = fsys_get_dir_entry(filesel_which);

   if(de->file_type == FILE_TYPE_DIRECTORY)
   {
    if(push_filesel())
    {
     if(!fsys_change_dir(filesel_which, filter))
     {
      snprintf(error_message, sizeof(error_message), "%s", fsys_get_error());
      pop_filesel();
     }
     else
     {
      filesel_which = 0;
      filesel_scroll = 0;
     }
    }
   }
   else
    return de;
  }
 }
 //
 //
 const uint32 total_count = fsys_num_dir_entries();
 int delta = 0;

 if(total_count)
 {
  //printf("%04x\n", buttons);
  if((buttons_np == BUTTON_MASK_UP || buttons_np == BUTTON_MASK_DOWN) && buttons == buttons_np)
  {
   filesel_which -= (buttons_np == BUTTON_MASK_UP);
   filesel_which += (buttons_np == BUTTON_MASK_DOWN);

   if(filesel_which < 0)
    filesel_which = total_count - (-filesel_which % total_count);

   if(filesel_which >= total_count)
    filesel_which %= total_count;
  }
  else
  {
   delta -=  1 * check_held(BUTTON_UP);
   delta -= 12 * check_held(BUTTON_X);
   delta -= 12 * 7 * check_held(BUTTON_L);

   delta += 1 * check_held(BUTTON_DOWN);
   delta += 12 * check_held(BUTTON_Y);
   delta += 12 * 7 * check_held(BUTTON_R);

   filesel_which += delta;

   if(filesel_which < 0)
    filesel_which = 0;

   if(filesel_which >= total_count)
    filesel_which = total_count - 1;
  }

  if(filesel_which < filesel_scroll)
   filesel_scroll = filesel_which;
  if(filesel_which >= (filesel_scroll + 13))
   filesel_scroll = filesel_which - (13 - 1);
 }

 gfx_begin_draw(hires_mode);

 const unsigned w = 40 << hires_mode;

 for(unsigned i = 0; i < 13; i++)
 {
  const int fli = filesel_scroll + i;

  if(fli < total_count)
  {
   unsigned color = 0x77;
   fsys_dir_entry_t* de = fsys_get_dir_entry(fli);
   char c = ' ';
   int tcol = 0x08;

   switch(de->file_type)
   {
    case FILE_TYPE_DIRECTORY:
	color = 0x22;
	break;

    default:
    case FILE_TYPE_UNKNOWN:
	break;

    case FILE_TYPE_NSF:
	c = 0x07;
	tcol = 0x06;
	break;

    case FILE_TYPE_NSFE:
	c = 0x03;
	tcol = 0x04;
	break;

    case FILE_TYPE_NES:
	c = 0x16;
	tcol = 0x08;
	break;

    case FILE_TYPE_TXT:
	c = 0x15;
	tcol = 0x01;
	break;
   }

   if(fli == filesel_which)
    tcol |= color & 0x70;
   //
   //
   gfx_draw_text_trunc_alt(1, 1 + i, w - 1, de->name, color & ((fli == filesel_which) ? 0x70 : 0x0F), 0x09 | ((fli == filesel_which) ? (color & 0x70) : 0));
   gfx_draw_char(0, 1 + i, c, tcol);
  }
 }

 if(!total_count)
 {
  const char* emptymsg = "Directory Empty";
  int x = (int)(w - strlen(emptymsg)) / 2;

  gfx_draw_text(x, 7, w, emptymsg, 0x20);
 }
 else
 {
  if((filesel_scroll + 13) < total_count)
   gfx_draw_char(w / 2, 14, 0x19, 0x08);
  if(filesel_scroll)
   gfx_draw_char(w / 2, 0, 0x18, 0x08);
 }

 gfx_update_ntptr();

 return NULL;
}

static bool load_text_file(nsfcore_file_callb_t* callb)
{
 unsigned y = 0;
 unsigned x = 0;
 char tmp[128];
 ssize_t r;
 char prev_c = 0;
 uint32 max_lines;

 text_buffer = (char*)nsfcore_get_rom_ptr(&text_buffer_size);
 max_lines = text_buffer_size / (80 + 1);

 memset(text_buffer, 0, max_lines * (80 + 1));
 text_num_lines = 0;
 text_scroll = 0;

 while((r = callb->read(callb, tmp, sizeof(tmp))) > 0)
 {
  for(int i = 0; i < r; i++)
  {
   char c = tmp[i];

   if(c == '\r' || c == '\n')
   {
    if((c == '\r' && prev_c != '\n') || (c == '\n' && prev_c != '\r'))
     x = 80;
   }
   else if(c == '\t')
   {
    for(int j = (8 - (x & 0x7)) & 0x7; j; j--)
    {
     text_buffer[y * (80 + 1) + x] = ' ';
     x++;
    }
   }
   else
   {
    text_num_lines = y + 1;
    text_buffer[y * (80 + 1) + x] = c;
    x++;
   }

   if(x == 80)
   {
    x = 0;
    y++;

    if(y >= max_lines)
     break;
   }

   prev_c = c;
  }
 }

 return true;
}

static bool run_text_viewer(void)
{
 const unsigned w = 40 << hires_mode;

 gfx_begin_draw(hires_mode);

 {
  int32 delta = 0;

  delta -=  1 * check_held(BUTTON_UP);
  delta -= 12 * check_held(BUTTON_X);
  delta -= 12 * 7 * check_held(BUTTON_L);

  delta += 1 * check_held(BUTTON_DOWN);
  delta += 12 * check_held(BUTTON_Y);
  delta += 12 * 7 * check_held(BUTTON_R);

  if(delta < 0 && (uint32)-delta > text_scroll)
   delta = -text_scroll;

  text_scroll += delta;

  if((text_scroll + 13) > text_num_lines)
   text_scroll = text_num_lines - ((13 > text_num_lines) ? text_num_lines : 13);
 }


 for(unsigned i = 0; i < 13; i++)
 {
  uint32 tbi = i + text_scroll;

  if(tbi < text_num_lines)
   gfx_draw_text_trunc_alt(0, 1 + i, w, text_buffer + tbi * (80 + 1), 0x07, 0x09);
 }

 if((13 + text_scroll) < text_num_lines)
  gfx_draw_char(w / 2, 14, 0x19, 0x08);
 if(text_scroll)
  gfx_draw_char(w / 2, 0, 0x18, 0x08);


 gfx_update_ntptr();

 return (buttons_held[BUTTON_START] == 2 || buttons_held[BUTTON_B] == 24);
}

static bool load_hex(nsfcore_file_callb_t* callb)
{
 int32 dr = 0;

 hex_buffer = (uint8*)nsfcore_get_rom_ptr(&hex_buffer_size);

 if((dr = callb->read(callb, hex_buffer, hex_buffer_size)) < 0)
  return false;

 hex_size = dr; 
 hex_num_lines = (hex_size + 0xF) >> 4;
 hex_scroll = 0;

 return true;
}

static bool run_hex_viewer(void)
{
 gfx_begin_draw(hires_mode);

 {
  int32 delta = 0;

  delta -=  1 * check_held(BUTTON_UP);
  delta -= 12 * check_held(BUTTON_X);
  delta -= 12 * 7 * check_held(BUTTON_L);

  delta += 1 * check_held(BUTTON_DOWN);
  delta += 12 * check_held(BUTTON_Y);
  delta += 12 * 7 * check_held(BUTTON_R);

  if(delta < 0 && (uint32)-delta > hex_scroll)
   delta = -hex_scroll;

  hex_scroll += delta;

  if((hex_scroll + 13) > hex_num_lines)
   hex_scroll = hex_num_lines - ((13 > hex_num_lines) ? hex_num_lines : 13);
 }

 //
 //
 //
 static const char hexlut[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
 uint32 addr = hex_scroll << 4;

 for(unsigned row = 0; row < 13; row++)
 {
  uint32 atmp = addr;
  for(unsigned i = 0; i < 8; i++)
  {
   gfx_draw_char(7 - i, 1 + row, hexlut[atmp & 0xF], (addr & 0x20) ? 0x17 : 0x09);
   atmp >>= 4;
  }

  for(unsigned i = 0; i < 16; i++)
  {
   const uint32 idx = addr + i;

   if(idx < hex_size)
   {
    uint8 b = hex_buffer[idx];

    gfx_draw_char(10 + i * 3 + (i >> 2) + 1, 1 + row, hexlut[b & 0xF], 0x07);
    gfx_draw_char(10 + i * 3 + (i >> 2) + 0, 1 + row, hexlut[b >> 4], 0x07);
   }
  }

//  gfx_draw_char(10 + 16 * 3 + 3, 1 + row, 0xB3, 0x08);

  for(unsigned i = 0; i < 16; i++)
  {
   const uint32 idx = addr + i;

   if(idx < hex_size)
    gfx_draw_char(10 + 16 * 3 + 3 + 2 + i, 1 + row, hex_buffer[idx], 0x03);
  }

  addr += 0x10;
 }

 if(addr < hex_size)
  gfx_draw_char(36, 14, 0x19, 0x08);
 if(hex_scroll)
  gfx_draw_char(36, 0, 0x18, 0x08);
 //
 //
 //
 gfx_update_ntptr();

 return (buttons_held[BUTTON_START] == 2 || buttons_held[BUTTON_B] == 24);
}

static void draw_init_status(const char* format, ...)
{
 const unsigned w = 40 << hires_mode;
 char tmp[80 + 1];
 va_list ap;
 unsigned x;
 size_t slen;

 va_start(ap, format);
 vsnprintf(tmp, sizeof(tmp), format, ap);
 va_end(ap);

 slen = strlen(tmp);
 if(slen > w)
  x = 0;
 else
  x = (w - slen) / 2;

 gfx_begin_draw(hires_mode);
 gfx_draw_text(x, 7, w - x, tmp, 0x9);

 {
  const char d[] = "CroNSF " CRONSF_VERSION;
  const size_t slen = strlen(d);

  gfx_draw_text(w - slen, 14, slen, d, 0x08);
 }

#if 0
#ifdef __GNUC__
 {
  static const char gcc_version[] = "gcc " __VERSION__;
  const size_t slen = strlen(gcc_version);

  gfx_draw_text(0, 14, slen, gcc_version, 0x08);
 }
#endif
#endif

 gfx_update_ntptr();
}

static ssize_t nsfcore_read(struct nsfcore_file_callb_t_* callb, void* ptr, size_t count)
{
 return fsys_read(ptr, count);
}

static int32 nsfcore_tell(struct nsfcore_file_callb_t_* callb)
{
 return fsys_tell();
}

static int nsfcore_seek(struct nsfcore_file_callb_t_* callb, int32 new_position)
{
 return fsys_seek(new_position);
}

static int32 nsfcore_size(struct nsfcore_file_callb_t_* callb)
{
 return fsys_size();
}

static void nsfcore_make_fsys_callb(nsfcore_file_callb_t* callb)
{
 memset(callb, 0, sizeof(nsfcore_file_callb_t));

 callb->read = nsfcore_read;
 callb->tell = nsfcore_tell;
 callb->seek = nsfcore_seek;
 callb->size = nsfcore_size;
}

static bool do_cdb_init_stuff(void)
{
 draw_init_status("Authenticating disc...");

 if(cdb_auth() < 0)
 {
  snprintf(error_message, sizeof(error_message), "Disc authentication failed!");
  return false;
 }    

 draw_init_status("Initializing disc stream...");
 cdb_stream_init();

 return true;
}


int main(void)
{
 bios_change_clock_speed(0x1);
 //
 //
 //
 //
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_RESDISA;
 while(SMPC_SF & 0x1);

 gfx_init();
 init_input();
 update_input();

 phase = PHASE_INIT;

 for(;;)
 {
  init_error_continue:;

  if(phase != PHASE_PLAYER)
  {
   SCU_IST = ~1;
   while(!(SCU_IST & 0x1));
   SCU_IST = ~1;
  }

  update_input();

  if(buttons_held[BUTTON_Z] == 2)
  {
   if(phase != PHASE_HEX_VIEWER)
   {
    hires_mode = !hires_mode;
    gfx_set_hires_mode(hires_mode);
   }
  }

  if(error_message[0])
  {
   const unsigned w = 40 << hires_mode;
   const size_t s = strlen(error_message);
   //
   gfx_begin_draw(hires_mode);

   for(int y = 0; y <= (s / w); y++)
   {
    int x = (int)(w - s) / 2;

    if(x < 0 || y > 0)
     x = 0;

    gfx_draw_text(x, 7 + y, w, error_message + (y * w), 0x0C);
   }
   gfx_update_ntptr();
   //
   //
   if(buttons_held[BUTTON_START] == 2)
    error_message[0] = 0;
  }
  else if(phase == PHASE_INIT)
  {
   const bool cart_fsys = (!bios_read_first_mode && !memcmp((uint8*)0x02000000 + 0x70, "CRONSF", 6));

   if(cart_fsys)
   {
    draw_init_status("Loading cart ISO file list...", 1);

    if(!fsys_init(0, (uint8*)0x02000000, 48 * 1024 * 1024, filter))
    {
     snprintf(error_message, sizeof(error_message), "Error loading cart filesystem: %s", fsys_get_error());
     continue;
    }
   }
   else
   {
    if(!do_cdb_init_stuff())
     continue;

    draw_init_status("Loading track %u ISO file list...", 1);
    {
     if(!fsys_init(1, NULL, 0, NULL))
     {
      snprintf(error_message, sizeof(error_message), "Error loading track %u filesystem: %s", 1, fsys_get_error());
      continue;
     }
    }
   }
   //
   //
   draw_init_status("Loading \"%s\"...", "font.bin");

   if(!fsys_open("font.bin"))
   {
    snprintf(error_message, sizeof(error_message), "Error opening \"%s\"!", "font.bin");
    continue;
   }

   gfx_load_font();
   //
   //
   draw_init_status("Initializing NSF core...");

   if(!nsfcore_init())
   {
    snprintf(error_message, sizeof(error_message), "Error initializing NSF core!");
    continue;
   }

   for(unsigned i = 0; i < 2; i++)
   {
    const char* fname = i ? "scsppal.bin" : "scspntsc.bin";

    draw_init_status("Loading \"%s\"...", fname);

    if(!fsys_open(fname))
    {
     snprintf(error_message, sizeof(error_message), "Error opening \"%s\"!", fname);
     goto init_error_continue;
    }

    if(!i)
    {
     nsfcore_file_callb_t callb;

     nsfcore_make_fsys_callb(&callb);

     if(!nsfcore_load_scsp_bin(&callb))
      goto init_error_continue;
    }
    else
    {
     if(fsys_read(scsp_ram_pal_init, 524288) != 524288)
     {
      snprintf(error_message, sizeof(error_message), "Error reading \"%s\"!", fname);
      goto init_error_continue;
     }
    }
   }
   //
   //
   //
   {
    const char* fname = "exchip.bin";
    nsfcore_file_callb_t callb;

    nsfcore_make_fsys_callb(&callb);

    draw_init_status("Loading \"%s\"...", fname);

    if(!fsys_open(fname))
    {
     snprintf(error_message, sizeof(error_message), "Error opening \"%s\"!", fname);
     goto init_error_continue;
    }

    if(!nsfcore_load_exchip_bin(&callb))
    {
     snprintf(error_message, sizeof(error_message), "Error loading \"%s\": %s", fname, nsfcore_get_error());
     goto init_error_continue;
    }
   }
   //
   //
   //
   {
    const char* dir = "nsfs";
    fsys_dir_entry_t* de = fsys_find_file(dir);

    if(de)
    {
     if(!fsys_change_dir_de(de, filter))
     {
      snprintf(error_message, sizeof(error_message), "Error changing directory to \"%s\": %s", dir, fsys_get_error());
      continue;
     }
    }
    else
    {
     unsigned nsf_track = 2;

     if(cart_fsys)
     {
      if(!do_cdb_init_stuff())
       continue;

      nsf_track = 1;
     }
     //
     draw_init_status("Loading track %u ISO file list...", nsf_track);

     if(!fsys_init(nsf_track, NULL, 0, filter))
     {
      snprintf(error_message, sizeof(error_message), "Error loading track %u filesystem: %s", nsf_track, fsys_get_error());
      continue;
     }
    }
   }

   phase = PHASE_FILE_SELECTOR;
  }
  else if(phase == PHASE_TEXT_VIEWER)
  {
   if(run_text_viewer())
    phase = PHASE_FILE_SELECTOR;
  }
  else if(phase == PHASE_HEX_VIEWER)
  {
   if(run_hex_viewer())
   {
    phase = PHASE_FILE_SELECTOR;
    //
    hires_mode = hires_mode_save;
    gfx_set_hires_mode(hires_mode);
   }
  }
  else if(phase == PHASE_FILE_SELECTOR)
  {
   fsys_dir_entry_t* de = run_file_selector();

   if(de)
   {
    const bool is_nsf = (de->file_type == FILE_TYPE_NSF || de->file_type == FILE_TYPE_NSFE);
    nsfcore_file_callb_t callb;

    nsfcore_make_fsys_callb(&callb);

    draw_init_status("Loading \"%s\"...", de->name);

    if(!fsys_open_de(de))
     snprintf(error_message, sizeof(error_message), "Error opening \"%s\"!", de->name);
    else if(buttons & BUTTON_MASK_C)
    {
     if(load_hex(&callb))
     {
      hires_mode_save = hires_mode;
      hires_mode = true;
      gfx_set_hires_mode(hires_mode);
      //
      phase = PHASE_HEX_VIEWER;
     }
    }
    else if(!is_nsf)
    {
     if(load_text_file(&callb))
      phase = PHASE_TEXT_VIEWER;
    }
    else
    {
     if(!(nsf_meta = nsfcore_load(&callb)))
     {
      //snprintf(error_message, sizeof(error_message), "Error loading \"%s\": %s", de->name, nsfcore_get_error());
      snprintf(error_message, sizeof(error_message), "%s", nsfcore_get_error());
     }
     else
     {
      if(!nsfcore_swapload_scsp_bin(scsp_ram_pal_init))
      {
       snprintf(error_message, sizeof(error_message), "%s", nsfcore_get_error());
       nsfcore_close();
       nsf_meta = NULL;
      }
      else
      {
       nsf_song_number = nsf_meta->song_start % nsf_meta->song_count;
       phase = PHASE_PLAYER;
       //
       //
       player_draw();

       nsfcore_set_song(nsf_song_number);
      }
     }
    }
   }
  }
  else
  {
   if(run_player())
    phase = PHASE_FILE_SELECTOR;
  }
 }

 return 0;
}

