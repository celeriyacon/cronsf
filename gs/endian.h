/*
 * gs/endian.h
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

#ifndef GIGASANTA_ENDIAN_H
#define GIGASANTA_ENDIAN_H

#include "types.h"

static INLINE uint16 read16_le(void* p)
{
 uint8* d = (uint8*)p;

 return (d[0] << 0) | (d[1] << 8);
}

static INLINE void write16_le(void* p, uint16 value)
{
 uint8* d = (uint8*)p;

 d[0] = value;
 d[1] = value >> 8;
}

static INLINE uint32 read32_le(void* p)
{
 uint8* d = (uint8*)p;

 return (d[0] << 0) | (d[1] << 8) | ((uint32)d[2] << 16) | ((uint32)d[3] << 24);
}

static INLINE uint64 read64_le(void* p)
{
 uint8* d = (uint8*)p;
 uint32 h, l;

 l = (d[0] << 0) | (d[1] << 8) | ((uint32)d[2] << 16) | ((uint32)d[3] << 24);
 h = (d[4] << 0) | (d[5] << 8) | ((uint32)d[6] << 16) | ((uint32)d[7] << 24);

 return ((uint64)h << 32) | l;
}

static INLINE uint16 read16_be(void* p)
{
 uint16 ret;

 __builtin_memcpy(&ret, p, sizeof(ret));

 return ret;
}

static INLINE uint32 read32_be(void* p)
{
 uint32 ret;

 __builtin_memcpy(&ret, p, sizeof(ret));

 return ret;
}

static INLINE uint64 read64_be(void* p)
{
 uint64 ret;

 __builtin_memcpy(&ret, p, sizeof(ret));

 return ret;
}

#endif
