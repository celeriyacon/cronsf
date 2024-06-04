/*
 * string.c
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

size_t strlen(const char* s)
{
 const char* p = s;

 while(*p)
  p++;

 return p - s;
}

void* memcpy(void* d, const void* s, size_t n)
{
 uintptr_t du = (uintptr_t)d;
 uintptr_t su = (uintptr_t)s;
 uintptr_t du_bound = du + n;

 if(!n)
  return d;

 if(n >= 16 && !((du ^ su) & 0x3))
 {
  size_t z_bound;

  z_bound = du + ((4 - (du & 0x3)) & 0x3);
  while(du < z_bound)
  {
   *(unsigned char*)du = *(unsigned char*)su;
   du++;
   su++;
  }

  z_bound = (du_bound &~ 0x3);
  while(du < z_bound)
  {
   *(__may_alias_uint32_t*)du = *(__may_alias_uint32_t*)su;
   du += 4;
   su += 4;
  }
 }

 while(du < du_bound)
 {
  *(unsigned char*)du = *(unsigned char*)su;
  du++;
  su++;
 }

 return d;
}

int strcmp(const char* s1, const char* s2)
{
 size_t i = 0;

 for(;;)
 {
  unsigned char c1 = *((unsigned char*)s1 + i);
  unsigned char c2 = *((unsigned char*)s2 + i);
  int diff = c1 - c2;

  if(diff)
   return diff;

  if(!(c1 | c2))
   break;

  i++;
 }

 return 0;
}

int strcasecmp(const char* s1, const char* s2)
{
 size_t i = 0;

 for(;;)
 {
  const unsigned char c1 = *((const unsigned char*)s1 + i);
  const unsigned char c2 = *((const unsigned char*)s2 + i);

  if(toupper(c1) != toupper(c2))
   return c1 - c2;

  if(!(c1 | c2))
   break;

  i++;
 }

 return 0;
}

int memcmp(const void* s1, const void* s2, size_t n)
{
 for(size_t i = 0; i < n; i++)
 {
  unsigned char c1 = *((unsigned char*)s1 + i);
  unsigned char c2 = *((unsigned char*)s2 + i);
  int diff = c1 - c2;

  if(diff)
   return diff;
 }

 return 0;
}

void* memset(void* s, int c, size_t n)
{
 uintptr_t su = (uintptr_t)s;
 const uintptr_t su_bound = su + n;

 if(n >= 16)
 {
  while(su < ((su_bound + 3) & ~3))
  {
   *(unsigned char*)su = c;
   su++;
  }
  uint32_t tmp;

  tmp = (unsigned char)c;
  tmp |= tmp << 8;
  tmp |= tmp << 16;

  while(su < (su_bound &~ 0x3))
  {
   *(__may_alias_uint32_t*)su = tmp;
   su += 4;
  }
 }

 while(su < su_bound)
 {
  *(unsigned char*)su = c;
  su++;
 }

 return s;
}


char *strrchr(const char *s, int c)
{
 if(!s[0])
  return NULL;

 for(size_t i = strlen(s) - 1; i != (size_t)-1; i--)
 {
  if(s[i] == (char)c)
   return (char*)(s + i);
 }

 return NULL;
}
