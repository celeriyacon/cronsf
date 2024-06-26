/*
 * stdlib.c
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

#include <stdlib.h>
#include <unistd.h>
#include "../gs/smpc.h"

static unsigned long long lcg[2];

int rand(void)
{
 lcg[0] = (lcg[0] * 6364136223846793005ULL) + 1442695040888963407ULL;
 lcg[1] = (lcg[1] * 0x5DEECE66D) + 0xB;

 return ((lcg[0] >> 32) ^ (lcg[1] >> 16)) & RAND_MAX;
}

void srand(unsigned int seed)
{
 lcg[0] = lcg[1] = seed;
}

void abort(void)
{
 static const char aborted_msg[] = "Aborted\n";

 write(2, aborted_msg, sizeof(aborted_msg));

 for(;;)
 {
  SMPC_COMREG = SMPC_CMD_SYSRES;
 }
}
