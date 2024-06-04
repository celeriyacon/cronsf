/*
 * gs/sh2.c
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

#include "types.h"
#include "sh2.h"

/*
void sh2_frt_wait_approx(uint32 cycles)
{
 uint32 prev_time;

 TCR = 0;	// /8
 TOCR = 0;

 prev_time = FRCH << 8;
 prev_time |= FRCL << 0;

 do
 {
  uint32 cur_time;
  uint32 tmp;

  //sh2_wait_approx((cycles > 100) ? 100 : cycles);

  // Order is important, so don't cram them into one statement!
  cur_time = FRCH << 8;
  cur_time |= FRCL << 0;

  tmp = (uint16)(cur_time - prev_time) << 3;
  cycles -= (tmp > cycles) ? cycles : tmp;
  prev_time = cur_time;
 } while(LIKELY(cycles > 0));
}
*/
