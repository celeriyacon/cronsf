/*
 * unistd.c
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

#include <unistd.h>
#include "../gs/scsp.h"

ssize_t write(int fd, const void *buf, size_t count)
{
 if(fd == 1 || fd == 2)
 {
  for(size_t i = 0; i < count; i++)
  {
   unsigned char c = *((unsigned char*)buf + i);

   while(SCSP_CREG(0x4) & 0x1000)
   {
    unsigned count = 2048;

    asm volatile(
	"dt %0\n\t"
	"1:\n\t"
	"bf/s 1b\n\t"
	"dt %0\n\t"
	: "+&r"(count)
	:
	: "cc");
   }

   SCSP_CREG_LO(0x6) = c;
  }

  return count;
 }

 //errno = EBADF;
 return -1;
}