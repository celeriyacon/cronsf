/*
 * gs/types.h
 *
 * Copyright (C) 2021 celeriyacon - https://github.com/celeriyacon
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

#ifndef GIGASANTA_TYPES_H
#define GIGASANTA_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define INLINE inline	__attribute__((always_inline))

#define UNLIKELY(v)	__builtin_expect((v) != 0, 0)
#define LIKELY(v)	__builtin_expect((v) != 0, 1)

#define NOWARN_UNUSED	__attribute__((unused))

// Unlimited cheese.
#define COLD_SECTION	__attribute__((section(".text.cold")))
#define LORAM_BSS	__attribute__((section(".loram.bss")))
#define LORAM_BSS_UNCACHED	__attribute__((section(".loram.bss.uncached")))
#define HIRAM_BSS_UNCACHED	__attribute__((section(".hiram.bss.uncached")))
#define VDP1_BSS_UNCACHED __attribute__((section(".vdp1.bss.uncached")))

#define VDP2_BSS __attribute__((section(".vdp2.bss")))
/* #define VDP2_BSS_UNCACHED __attribute__((section(".vdp2.bss.uncached"))) */

#define ALIGN(n)	__attribute__((aligned(n)))

#define MAY_ALIAS 	__attribute__((__may_alias__))

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint32_t MAY_ALIAS uint32_MA;

#endif
