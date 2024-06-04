#ifndef GIGASANTA_STDINT_H
#define GIGASANTA_STDINT_H

#include <stddef.h>

#if defined(__sh2__)
//
typedef signed int intptr_t;
typedef unsigned int uintptr_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
//
#elif defined( __m68k__)
//
typedef signed long intptr_t;
typedef unsigned long uintptr_t;

typedef signed long long intmax_t;
typedef unsigned long long uintmax_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef signed long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;
//
#endif

typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

#define __INTN_MIN(b)	((int ## b ## _t) ((uint ## b ## _t)1 << (b - 1))     )
#define __INTN_MAX(b)	((int ## b ## _t)(((uint ## b ## _t)1 << (b - 1)) - 1))

#define INT8_MIN	__INTN_MIN(8)
#define INT8_MAX	__INTN_MAX(8)

#define INT16_MIN	__INTN_MIN(16)
#define INT16_MAX	__INTN_MAX(16)

#define INT32_MIN	__INTN_MIN(32)
#define INT32_MAX	__INTN_MAX(32)

#define INT64_MIN	__INTN_MIN(64)
#define INT64_MAX	__INTN_MAX(64)

#undef __INTN_MIN
#undef __INTN_MAX
//
//
#define UINT8_MIN	((uint8_t)0)
#define UINT8_MAX	((uint8_t)-1)

#define UINT16_MIN	((uint16_t)0)
#define UINT16_MAX	((uint16_t)-1)

#define UINT32_MIN	((uint32_t)0)
#define UINT32_MAX	((uint32_t)-1)

#define UINT64_MIN	((uint64_t)0)
#define UINT64_MAX	((uint64_t)-1)
//
//
#define SIZE_MAX	((size_t)-1)


typedef uint16_t __attribute__((__may_alias__)) __may_alias_uint16_t;
typedef uint32_t __attribute__((__may_alias__)) __may_alias_uint32_t;


#endif
