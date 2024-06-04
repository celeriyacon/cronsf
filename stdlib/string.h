#ifndef _STDLIB_STRING_H
#define _STDLIB_STRING_H

#include <stddef.h>

size_t strlen(const char *s);
char *strcpy(char *d, const char *s);
void *memcpy(void *d, const void *s, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memset(void *s, int c, size_t n);
char *strerror(int errnum);
int strcmp(const char *s1, const char *s2);
int strcasecmp(const char *s1, const char *s2);
char *strrchr(const char *s, int c);
#endif
