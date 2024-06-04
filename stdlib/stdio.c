/*
 * stdio.c
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

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

struct _FILE
{
 int fd;
 bool error;
 bool eof;
};

static FILE _stdin = { 0 };
static FILE _stdout = { 1 };
static FILE _stderr = { 2 };

FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

int fileno(FILE *stream)
{
 return stream->fd;
}

void clearerr(FILE *stream)
{
 stream->error = false;
 stream->eof = false;
}

int feof(FILE *stream)
{
 return stream->eof;
}

int ferror(FILE *stream)
{
 return stream->error;
}

long ftell(FILE *stream)
{
 errno = ENOSYS;
 return -1;
}

int fseek(FILE *stream, long offset, int whence)
{
 errno = ENOSYS;
 return -1;

 //  clearerr(stream);
}

int fclose(FILE *stream)
{
 if(stream == &_stdin || stream == &_stdout || stream == &_stderr)
 {
  errno = EINVAL;
  return EOF;
 }

 errno = ENOSYS;
 return -1;

 return 0;
}

FILE *fopen(const char *pathname, const char *mode)
{
/*
 FILE *ret = malloc(sizeof(FILE));

 if(!ret)
  return NULL;
*/

 errno = ENOSYS;
 return NULL;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
 errno = ENOSYS;
 stream->error = true;

 return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
 if(!size || !nmemb)
  return 0;
 //
 const size_t tsize = size * nmemb;

 if((tsize / size) != nmemb)
 {
  stream->error = true;
  return 0;
 }

 const ssize_t dw = write(fileno(stream), ptr, tsize);

 if(dw == -1)
 {
  stream->error = true;
  return 0;
 }

 if((size_t)dw != tsize)
  stream->error = true;

 return (size_t)dw / size;
}

int putc(int c, FILE *stream)
{
 unsigned char uc = c;

 if(fwrite(&uc, sizeof(uc), 1, stream) != 1)
  return EOF;

 return uc;
}

int fputc(int c, FILE *stream)
{
 return putc(c, stream);
}

int putchar(int c)
{
 return putc(c, stdout);
}

int fputs(const char *s, FILE *stream)
{
 const size_t s_len = strlen(s);

 if(fwrite(s, 1, s_len, stream) != s_len)
  return EOF;

 return 1;
}

int puts(const char *s)
{
 int r = fputs(s, stdout);

 if(r < 0)
  return r;

 return putchar('\n');
}

