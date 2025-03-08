/*
 * gfx.c
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

#include "config.h"

#include "gs/types.h"
#include "gs/vdp2.h"
#include "gs/scu.h"
#include "gs/cdb.h"

#include "gfx.h"
#include "fsys.h"

#include <string.h>

static unsigned nt_addr = 0x00000;
static const unsigned cg_addr = 0x10000;
static volatile uint32* nt_ptr;
static unsigned draw_x_base;
static bool hires_mode;
static bool hires_draw;

void gfx_set_draw(bool hires)
{
 hires_draw = hires;
 nt_ptr = (volatile uint32 *)&VDP2_VRAM[nt_addr + (hires << 15)];
 draw_x_base = 2 << hires;
 //
 //
 for(unsigned i = 0; i < 128 * 64; i += 2)
 {
  nt_ptr[i + 0] = 0;
  nt_ptr[i + 1] = 0;
 }
}

void gfx_begin_draw(bool hires)
{
#if 0
 CLOFEN = ~0;
 COAR = 0x20;
#endif

 nt_addr ^= 0x4000;
 gfx_set_draw(hires);
}

void gfx_update_ntptr(void)
{
 for(unsigned i = 0; i < 2 * 4; i++)
 {
  unsigned tmp = ((nt_addr + (hires_mode << 15)) >> (13 + 0)) & 0x3F;
  *(volatile uint16*)(0x25F80040 + (i << 1)) = (tmp << 8) | tmp;
 }
#if 0
 CLOFEN = 0;
#endif
}

static INLINE void gfx_draw_char_INLINE(unsigned x, unsigned y, const char c, uint8 color)
{
 const unsigned adj_x = draw_x_base + x;
 volatile uint32* p = nt_ptr + (((y << 1) + (adj_x & 0x40)) << 6) + (adj_x & 0x3F);
 uint32 tmp = (color << 16) + (cg_addr >> 4) + ((unsigned char)c << 1);

 p[0x00] = tmp;
 p[0x40] = tmp + 1;
}

void gfx_draw_char(unsigned x, unsigned y, const char c, uint8 color)
{
 gfx_draw_char_INLINE(x, y, c, color);
}

void gfx_draw_text(unsigned x, unsigned y, unsigned max_w, const char* text, uint8 color)
{
 const char* p = text;
 char c;
 unsigned i = 0;

 while((c = *p) && (i < max_w))
 {
  gfx_draw_char_INLINE(x, y, c, color);
  x++;
  p++;
  i++;
 }
}

void gfx_draw_text_trunc(unsigned x, unsigned y, unsigned max_w, const char* text, uint8 color, uint8 trunc_color)
{
 const char* p = text;
 char c;
 unsigned i = 0;

 while((c = *p) && (i < max_w))
 {
  gfx_draw_char_INLINE(x, y, c, color);
  x++;
  p++;
  i++;
 }

 if(*p != 0)
  gfx_draw_char(x, y, 0x1D, trunc_color);
}

void gfx_draw_text_trunc_alt(unsigned x, unsigned y, unsigned max_w, const char* text, uint8 color, uint8 trunc_color)
{
 const char* p = text;
 char c;
 unsigned i = 0;

 while((c = *p) && (i < max_w))
 {
  char ec = c;
  uint8 ecolor = color;

  if(i == (max_w - 1) && p[1] != 0)
  {
   ec = 0x1D;
   ecolor = trunc_color;
  }
  gfx_draw_char_INLINE(x, y, ec, ecolor);
  x++;
  p++;
  i++;
 }
}


void gfx_draw_box(unsigned x, unsigned y, unsigned w, unsigned h, const char* title)
{
 if(!w || !h)
  return;
 //
 unsigned border_color = 0x09;
 unsigned title_color = 0x05;
 unsigned thingy_color = 0x08;
 unsigned bg_color = 0x00;

 gfx_draw_char(x, y, 0xDA, border_color);
 gfx_draw_char(x + w - 1, y, 0xBF, border_color);
 gfx_draw_char(x + w - 1, y + h - 1, 0xD9, border_color);
 gfx_draw_char(x, y + h - 1, 0xC0, border_color);

 for(int i = 1; i < (h - 1); i++)
 {
  gfx_draw_char(x, y + i, 0xB3, border_color);
  gfx_draw_char(x + w - 1, y + i, 0xB3, border_color);

  for(int j = 1; j < (w - 2); j++)
   gfx_draw_char(x + j, y + i, ' ', bg_color);
 }

 for(int i = 1; i < (w - 1); i++)
 {
  gfx_draw_char(x + i, y, 0xC4, border_color);
  gfx_draw_char(x + i, y + (h - 1), 0xC4, border_color);
 }

 gfx_draw_char(x + 1, y, 0xAE, thingy_color);
 gfx_draw_text(x + 2, y, 36 - x, title, title_color);
 gfx_draw_char(x + 2 + strlen(title), y, 0xAF, thingy_color);
}

void gfx_init(void)
{
 for(unsigned i = 0x25F80000; i < 0x25F80200; i += 2)
  *(volatile uint16*)i = 0;

 for(unsigned i = 0; i < 2048; i++)
  CRAM[i] = 0;
 //
 //
 SCU_IST = ~1;
 while(!(SCU_IST & 0x1));

 TVMD = (1U << 15) |    // DISP
        (0x00 << 6) |   // LSMD
        (0x01 << 4) |   // VRES
        (0x01 << 0);    // HRES
 hires_mode = false;
 hires_draw = false;

 BGON = (1 << 10) | (1 << 2);
 PRINB = (0x7 << 0);
 CHCTLB = (0x0 << 0) | // N2CHSZ
          (0x0 << 1) | // N2CHCN (character color number)
          0;

 PNCN2 = (0x0 << 15) |  // N2PNB(pattern name data size; 0 = 2 words, 1 = 1 word)
          0;

 PLSZ = (0x1 << 4); // N2PLSZ
 //
 //
 for(unsigned n = 0; n < 4; n++)
 {
  MPOFN = ((nt_addr >> (19 + 0)) << (n * 4));
 }

 gfx_update_ntptr();

 for(unsigned y = 0; y < 32; y++)
 {
  for(unsigned x = 0; x < 32; x++)
  {
   volatile uint16* p = &VDP2_VRAM[nt_addr + (y * 64 + x) * 2];

   p[0] = 0;
   p[1] = (cg_addr >> 4) + (x << 1) + (y & 1) + ((y >> 1) << 6);
  }
 }

 CRAOFA = 0;

 // Scroll
 SCXN2 = 0;
 SCYN2 = 0;

 RAMCTL = (CRAM_MODE_RGB555_2048 << 12) | (0U << 9) | (0U << 8);

 for(unsigned i = 0; i < 16 * 8; i++)
 {
  int fg_r, fg_g, fg_b;
  int bg_r, bg_g, bg_b;

  fg_r = 20 * ((i >> 2) & 0x1) + 10 * ((i >> 3) & 0x1);
  fg_g = (20 * ((i >> 1) & 0x1) + 10 * ((i >> 3) & 0x1)) >> ((i & 0xF) == 0x6);
  fg_b = 20 * ((i >> 0) & 0x1) + 10 * ((i >> 3) & 0x1);

  bg_r = 20 * ((i >> 6) & 0x1);
  bg_g = 20 * ((i >> 5) & 0x1) >> (((i >> 4) & 0xF) == 0x6);
  bg_b = 20 * ((i >> 4) & 0x1);

  CRAM[(i << 4) + 0] = (bg_r << 0) | (bg_g << 5) | (bg_b << 10);
  CRAM[(i << 4) + 1] = (fg_r << 0) | (fg_g << 5) | (fg_b << 10);
 }

 {
  unsigned vcp[4][8];

  for(unsigned i = 0; i < 4; i++)
  {
   for(unsigned j = 0; j < 8; j++)
    vcp[i][j] = ((i & 1) ? VCP_NOP : VCP_CPU);
  }

  vcp[(cg_addr >> 16) &~ 1][0] = VCP_NBG2_CG;
  vcp[(nt_addr >> 16) &~ 1][1] = VCP_NBG2_NT;

  for(unsigned region = 0; region < 4; region++)
  {
   (*(volatile uint16*)(0x25F80010 + (region * 4))) = ((vcp[region][0] & 0xF) << 12) | ((vcp[region][1] & 0xF) << 8) | ((vcp[region][2] & 0xF) << 4) | ((vcp[region][3] & 0xF) << 0);
   (*(volatile uint16*)(0x25F80012 + (region * 4))) = ((vcp[region][4] & 0xF) << 12) | ((vcp[region][5] & 0xF) << 8) | ((vcp[region][6] & 0xF) << 4) | ((vcp[region][7] & 0xF) << 0);
  }
 }
}

bool gfx_load_font(void)
{
 volatile uint16* p = &VDP2_VRAM[cg_addr];
 uint8 buf[64];

 // Install font into VDP2 RAM
 for(unsigned i = 0; i < 4096; i += sizeof(buf))
 {
  if(fsys_read(buf, sizeof(buf)) != sizeof(buf))
   return false;

  for(unsigned j = 0; j < sizeof(buf); j++)
  {
   uint8 c = buf[j];
   uint16 tmp[2] = { 0, 0 };

   for(unsigned k = 0; k < 4; k++)
   {
    tmp[0] <<= 4;
    tmp[1] <<= 4;
    tmp[0] |= ((c >> 7) & 1);
    tmp[1] |= ((c >> 3) & 1);
    c <<= 1;
   }

   p[0] = tmp[0];
   p[1] = tmp[1];
   p += 2;
  }
 }

 return true;
}

void gfx_set_hires_mode(bool hr)
{
 hires_mode = hr;

 TVMD = (TVMD & ~0x0002) | (hires_mode << 1);
 gfx_update_ntptr();
}

