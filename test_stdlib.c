/*
 * test_stdlib.c
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

#include "gs/types.h"
#include "gs/sh2.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>

static void test_string(void)
{
 static const struct string_test_vector_t_
 {
  const char* format;
  const char* arg;
  const char* expected;
 } tvs[] =
 {
  { "Test %s",   "ABC",		"Test ABC" },

  { "Test %.3s", "ABCDEFG",	"Test ABC" },
  { "Test %.5s", "ABC\0DEFG",	"Test ABC" },
  { "Test %.0s", "ABC",		"Test " },

  { "Test %3s too", "c",	"Test   c too" },
  { "Test %03s too", "c",	"Test   c too" },
  { "Test %-3s too", "c",	"Test c   too" },

  { "Test %4.3s too", "ABCDEFG", "Test  ABC too" },
  { "Test %2.3s too", "ABCDEFG", "Test ABC too" },
  { "Test %-4.3s too", "ABCDEFG", "Test ABC  too" },
  { "Test %-2.3s too", "ABCDEFG", "Test ABC too" },

  { "%s", "", "" },
  { "%s", "A", "A" },
 };

 for(size_t i = 0; i < sizeof(tvs) / sizeof(tvs[0]); i++)
 {
  const struct string_test_vector_t_* tv = &tvs[i];
  char tmp[256] = { 0 };

  snprintf(tmp, sizeof(tmp), tv->format, tv->arg);

  if(strcmp(tmp, tv->expected))
  {
   printf("Format string \"%s\" argument \"%s\" wrong result: %s\n", tv->format, tv->arg, tmp);
   abort();
  }

  for(size_t j = strlen(tmp); j < sizeof(tmp); j++)
  {
   assert(!tmp[j]);
  }
 }
}

static void test_int(void)
{
 static const struct int_test_vector_t_
 {
  const char* format;
  int arg;
  const char* expected;
 } tvs[] =
 {
  { "%d", 0,			"0" },
  { "%0d", 0,			"0" },
  { "%3d", 0,			"  0" },
  { "%03d", 0,			"000" },
  { "%d", -1,			"-1" },
  { "%3d", -1,			" -1" },
  { "%+3d", -1,			" -1" },
  { "%+3d", 1,			" +1" },
  { "% 3d", -1,			" -1" },
  { "% 3d", 1,			"  1" },
  { "% 03d", 1,			" 01" },
  { "% 03d", -1,		"-01" },

  { "%.0d", 0,			"" },
  { "%.0d", 1,			"1" },
  { "%.4d", 1,			"0001" },
  { "%.4d", -1,			"-0001" },

  { "%5.4d", 1,			" 0001" },
  { "%5.4d", -1,		"-0001" },

  { "%+5.4d", 1,		"+0001" },
  { "%+6.4d", 1,		" +0001" },
  { "%+06.4d", 1,		"+00001" },

  { "%-+06.4d", 1,		"+0001 " },
  { "%-+06.4d", -1,		"-0001 " },

  { "%u", 0,			"0" },
  { "%3u", 0,			"  0" },
  { "%03u", 0,			"000" },

  { "%u", 1234567,		"1234567" },
  { "%8x", 0xBEEF,		"    beef" },
  { "%8X", 0xBEEF,		"    BEEF" },
  { "%#16x", 1,			"             0x1" },
  { "%#016x", 1,		"0x00000000000001" },
  { "%#16X", 1,			"             0X1" },
  { "%#016X", 1,		"0X00000000000001" },
  { "%#1x", 9,			"0x9" },
 };

 for(size_t i = 0; i < sizeof(tvs) / sizeof(tvs[0]); i++)
 {
  const struct int_test_vector_t_* tv = &tvs[i];
  char tmp[256] = { 0 };

  snprintf(tmp, sizeof(tmp), tv->format, tv->arg);

  if(strcmp(tmp, tv->expected))
  {
   printf("Format string \"%s\" argument \"%d\" wrong result \"%s\"\n", tv->format, tv->arg, tmp);
   abort();
  }

  for(size_t j = strlen(tmp); j < sizeof(tmp); j++)
  {
   assert(!tmp[j]);
  }
 }
}

int main(int argc, char* argv[])
{
 printf("Start!\n");

 test_string();
 test_int();

 printf("Done.\n");

 sh2_wait_approx(100000);

 return 0;
}
