#ifndef _STDLIB_STDLIB_H
#define _STDLIB_STDLIB_H

#include <stddef.h>

#define RAND_MAX ((int)(((unsigned)-1) >> 1))

int rand(void);
void srand(unsigned int seed);
void abort(void);

#endif
