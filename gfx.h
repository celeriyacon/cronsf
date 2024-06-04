/*
 * gfx.h
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

#ifndef GFX_H
#define GFX_H

void gfx_set_draw(bool hires);
void gfx_begin_draw(bool hires);
void gfx_update_ntptr(void);
void gfx_draw_char(unsigned x, unsigned y, const char c, uint8 color);
void gfx_draw_text(unsigned x, unsigned y, unsigned max_w, const char* text, uint8 color);
void gfx_draw_text_trunc(unsigned x, unsigned y, unsigned max_w, const char* text, uint8 color, uint8 trunc_color);
void gfx_draw_text_trunc_alt(unsigned x, unsigned y, unsigned max_w, const char* text, uint8 color, uint8 trunc_color);
void gfx_draw_box(unsigned x, unsigned y, unsigned w, unsigned h, const char* title);
void gfx_init(void);
bool gfx_load_font(void);
void gfx_set_hires_mode(bool hr);

#endif
