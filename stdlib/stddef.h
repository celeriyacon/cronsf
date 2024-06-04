#ifndef GIGASANTA_STDDEF_H
#define GIGASANTA_STDDEF_H

#define NULL ((void*)0)

#if defined(__sh2__)
typedef unsigned int size_t;
typedef signed int ssize_t;
typedef signed int ptrdiff_t;
#endif

#endif
