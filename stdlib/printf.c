/*
 * printf.c
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

// FIXME: INT_MAX vs size_t, size_t overflow

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>

#undef NDEBUG
#include <assert.h>

typedef struct __fstrcontext_t_
{
 bool (*ofnp)(struct __fstrcontext_t_* ctx, char c);
 bool (*osfnp)(struct __fstrcontext_t_* ctx, const char* s);

 size_t char_counter;
 void* p;
 size_t p_size;
 //
 bool flag_left_justified;
 bool flag_plus;
 bool flag_space;
 bool flag_alternate;
 bool flag_zero;

 bool precision_valid;

 int field_width;
 int precision;
 unsigned length_modifier;
} __fstrcontext_t;

static bool output_char(__fstrcontext_t* ctx, char c)
{
 if(putc(c, (FILE*)ctx->p) == EOF)
  return false;

 ctx->char_counter++;

 return true;
}

static bool output_string(__fstrcontext_t* ctx, const char* s)
{
 FILE* stream = (FILE*)ctx->p;
 const size_t slen = strlen(s);
 size_t dw;

 clearerr(stream);

 dw = fwrite(s, 1, slen, stream);

 ctx->char_counter += dw;

 if(ferror(stream))
  return false;

 return true;
}

static bool append_char(__fstrcontext_t* ctx, char c)
{
 if(ctx->char_counter < ctx->p_size)
  ((char*)ctx->p)[ctx->char_counter] = c;

 ctx->char_counter++;

 return true;
}

static bool append_string(__fstrcontext_t* ctx, const char* const s)
{
 const char* si = s;
 char c;

 while((c = *si))
 {
  if(ctx->char_counter < ctx->p_size)
   ((char*)ctx->p)[ctx->char_counter] = c;

  ctx->char_counter++;
  si++;
 }

 return true;
}

static bool process_string(__fstrcontext_t* ctx, const char* s)
{
 if(!s)
  s = "(null)";
 //
 size_t slen = strlen(s);
 unsigned padc = 0;

 if(ctx->precision_valid)
 {
  if(ctx->precision <= 0)
   return true;

  if(slen > (unsigned)ctx->precision)
   slen = ctx->precision;
 }

 if(ctx->field_width > 0 && slen < (unsigned)ctx->field_width)
  padc = ctx->field_width - slen;

 if(!ctx->flag_left_justified && padc)
 {
  do
  {
   if(!ctx->ofnp(ctx, ' '))
    return false;
  } while(--padc);
 }

 for(size_t i = 0; i < slen; i++)
 {
  const char c = s[i];
  
  if(!ctx->ofnp(ctx, c))
   return false;
 }

 if(ctx->flag_left_justified && padc)
 {
  do
  {
   if(!ctx->ofnp(ctx, ' '))
    return false;
  } while(--padc);
 }

 return true;
}

static bool process_unsigned(__fstrcontext_t* ctx, const unsigned long long v_in, const unsigned base, const bool uppercase, char prefix_char)
{
 unsigned long long v = v_in;
 int field_width = ctx->field_width;
 int precision = (ctx->precision_valid ? ctx->precision : 0);
 char buf[64 + 1];
 char* d = buf + sizeof(buf) - 1;
 bool ret;

 buf[sizeof(buf) - 1] = 0;

 if(!v && field_width <= 0)
  return true;
 //
 //
 do
 {
  assert(d != buf);
  d--;
  //
  unsigned t = v % base;
  char c;

  if(t < 10)
   c = '0' + t;
  else
   c = (uppercase ? 'A' : 'a') + (t - 10);

  *d = c;
  v /= base;
  field_width -= (field_width > 0);
  precision -= (precision > 0);
 } while(v);

 field_width -= ((field_width < precision) ? field_width : precision);

 if(prefix_char && ctx->flag_zero)
 {
  if(!ctx->ofnp(ctx, prefix_char))
   return false;

  field_width -= (field_width > 0);
  prefix_char = 0;
 }

 if(ctx->flag_alternate && base == 16)
 {
  if(!ctx->flag_left_justified && !ctx->flag_zero)
  {
   while(field_width > (2 + (bool)prefix_char))
   {
    if(!ctx->ofnp(ctx, ' '))
     return false;

    field_width--;
   }
  }

  if(!ctx->ofnp(ctx, '0'))
   return false;

  if(!ctx->ofnp(ctx, uppercase ? 'X' : 'x'))
   return false;

  field_width = ((field_width < 2) ? 0 : (field_width - 2));
 }

 if(!ctx->flag_left_justified)
 {
  while(field_width > (bool)prefix_char)
  {
   if(!ctx->ofnp(ctx, ctx->flag_zero ? '0' : ' '))
    return false;

   field_width--;
  }
 }

 if(prefix_char && !ctx->flag_zero)
 {
  if(!ctx->ofnp(ctx, prefix_char))
   return false;

  field_width -= (field_width > 0);
  prefix_char = 0;
 }

 while(precision > 0)
 {
  if(!ctx->ofnp(ctx, '0'))
   return false;

  precision--;
 }

 ret = ctx->osfnp(ctx, d);

 if(ctx->flag_left_justified)
 {
  while(field_width > 0)
  {
   if(!ctx->ofnp(ctx, ' '))
    return false;

   field_width--;
  }
 }

 return ret;
}


static bool process_signed(__fstrcontext_t* ctx, long long v, unsigned base, bool uppercase)
{
 char prefix_char = 0;

 if(v < 0)
 {
  prefix_char = '-';

  v = -(unsigned long long)v;
 }
 else if(ctx->flag_plus)
  prefix_char = '+';
 else if(ctx->flag_space)
  prefix_char = ' ';

 return process_unsigned(ctx, v, base, uppercase, prefix_char);
}

enum
{
 INSPEC_FLAGS = 1,
 INSPEC_FIELD_WIDTH,
 INSPEC_FIELD_WIDTH_EXT,
 INSPEC_PRECISION,
 INSPEC_PRECISION_EXT,
 INSPEC_LENGTH_MOD,
 INSPEC_CONV_SPEC,

 INSPEC_NONE = 0x7F
};

static int _Xprintf(__fstrcontext_t* ctx, const char* format, va_list ap)
{
 const char* s = format;
 char c;
 int in_spec = INSPEC_NONE;

 while((c = *s))
 {
  if(c == '%')
  {
   if(in_spec != INSPEC_NONE)
   {
    ctx->ofnp(ctx, c);
    in_spec = INSPEC_NONE;
   }
   else
   {
    in_spec = INSPEC_FLAGS;
    //
    ctx->flag_left_justified = false;
    ctx->flag_plus = false;
    ctx->flag_space = false;
    ctx->flag_alternate = false;
    ctx->flag_zero = false;
    //
    ctx->field_width = 1;
    ctx->precision = 0;
    ctx->precision_valid = false;
    ctx->length_modifier = 0;
   }
  }
  else if(in_spec != INSPEC_NONE)
  {
   bool handled = false;

   if(in_spec <= INSPEC_FLAGS)
   {
    switch(c)
    {
     case '-': ctx->flag_left_justified = true; handled = true; break;
     case '+': ctx->flag_plus = true; handled = true; break;
     case ' ': ctx->flag_space = true; handled = true; break;
     case '#': ctx->flag_alternate = true; handled = true; break;
     case '0': ctx->flag_zero = true;  handled = true; break;
    }

    if(handled)
     in_spec = INSPEC_FLAGS;
   }

   if(!handled && in_spec <= INSPEC_FIELD_WIDTH)
   {
    if(isdigit(c))
    {
     if(in_spec != INSPEC_FIELD_WIDTH)
     {
      ctx->field_width = 0;
      in_spec = INSPEC_FIELD_WIDTH;
     }

     ctx->field_width = (ctx->field_width * 10) + (c - '0');

     handled = true;
    }
    else if(c == '*' && in_spec != INSPEC_FIELD_WIDTH)
    {
     ctx->field_width = va_arg(ap, int);
     handled = true;
     in_spec = INSPEC_FIELD_WIDTH_EXT;
    }
   }

   if(!handled && in_spec <= INSPEC_PRECISION)
   {
    if(c == '.')
    {
     if(in_spec != INSPEC_PRECISION)
     {
      ctx->precision = 0;
      ctx->precision_valid = true;
      in_spec = INSPEC_PRECISION;
      handled = true;
     }
    }
    else if(in_spec == INSPEC_PRECISION)
    {
     if(c == '*')
     {
      ctx->precision = va_arg(ap, int);
      in_spec = INSPEC_PRECISION_EXT;
      handled = true;
     }
     else if(isdigit(c))
     {
      ctx->precision = (ctx->precision * 10) + (c - '0'); 
      handled = true;
     }
    }
   }

   if(!handled && in_spec <= INSPEC_LENGTH_MOD)
   {
    if(c == 'h' || c == 'l' || c == 'j' || c == 'z' || c == 't' || c == 'L')
    {
     ctx->length_modifier = (ctx->length_modifier << 8) | c;

     switch(ctx->length_modifier)
     {
      default:
	in_spec = INSPEC_NONE;
	break;

      case ('h' << 8) | 'h':
      case 'h':
      case ('l' << 8) | 'l':
      case 'l':
      case 'j':
      case 'z':
      case 't':
      case 'L':
	handled = true;
	in_spec = INSPEC_LENGTH_MOD;
	break;
     }
    }
   }

   if(!handled && in_spec <= INSPEC_CONV_SPEC)
   {
    if(c == 's')
    {
     if(!process_string(ctx, va_arg(ap, const char*)))
      return -1;
    }
    else if(c == 'c')
    {
     if(!ctx->ofnp(ctx, (char)va_arg(ap, int)))
      return -1;
    }
    else if(c == 'd' || c == 'i' || c == 'o' || c == 'u' || c == 'x' || c == 'X')
    {
     unsigned long long v = 0;
     const bool spec_signed = (c == 'd' || c == 'i');

     switch(ctx->length_modifier)
     {
      default:
	v = va_arg(ap, int);
	if(!spec_signed)
	 v = (unsigned int)v;
	break;

      case ('h' << 8) | 'h':
	v = (signed char)va_arg(ap, int);
	if(!spec_signed)
	 v = (unsigned char)v;
	break;

      case 'h':
	v = (short)va_arg(ap, int);
	if(!spec_signed)
	 v = (unsigned short)v;
	break;

      case ('l' << 8) | 'l':
	v = va_arg(ap, long long);
	break;

      case 'l':
	v = va_arg(ap, long);

	if(!spec_signed)
	 v = (unsigned long)v;
	break;

      //case 'j': v = va_arg(ap, intmax_t); break;
      case 'z':
	v = va_arg(ap, ssize_t);
	if(!spec_signed)
	 v = (size_t)v;
	break;
       //case 't':
       //case 'L':break;
     }

     switch(c)
     {
      case 'd':
      case 'i':
	if(!process_signed(ctx, v, 10, false))
	 return -1;
	break;

      case 'u':
	if(!process_unsigned(ctx, v, 10, false, 0))
	 return -1;
	break;

      case 'o':
	if(!process_unsigned(ctx, v, 8, false, 0))
	 return -1;
	break;

      case 'x':
      case 'X':
	if(!process_unsigned(ctx, v, 16, c == 'X', 0))
	 return -1;
	break;
     }
    }

    in_spec = INSPEC_NONE;
   }
  }
  else
   if(!ctx->ofnp(ctx, c))
    return -1;

  s++;
 }

 return ctx->char_counter;
}

int vfprintf(FILE *stream, const char *format, va_list ap)
{
 __fstrcontext_t ctx = { output_char, output_string, 0, stream, 0 };
 int ret;

 ret = _Xprintf(&ctx, format, ap);

 return ret;
}

int vprintf(const char *format, va_list ap)
{
 return vfprintf(stdout, format, ap);
}

int fprintf(FILE *stream, const char *format, ...)
{
 va_list ap;
 int ret;

 va_start(ap, format);
 ret = vfprintf(stream, format, ap);
 va_end(ap);

 return ret;
}

int printf(const char *format, ...)
{
 va_list ap;
 int ret;

 va_start(ap, format);
 ret = vprintf(format, ap);
 va_end(ap);

 return ret;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
 if(!size)
  return 0;
 //
 __fstrcontext_t ctx = { append_char, append_string, 0, str, size };
 int ret;

 ret = _Xprintf(&ctx, format, ap);

 if(ret >= 0)
 {
  size_t o = (ret > (size - 1)) ? size - 1 : ret;

  str[o] = 0;
 }

 return ret;
}

int vsprintf(char *str, const char *format, va_list ap)
{
 return vsnprintf(str, SIZE_MAX, format, ap);
}


int snprintf(char *str, size_t size, const char *format, ...)
{
 va_list ap;
 int ret;

 va_start(ap, format);
 ret = vsnprintf(str, size, format, ap);
 va_end(ap);

 return ret;
}

int sprintf(char *str, const char *format, ...)
{
 va_list ap;
 int ret;

 va_start(ap, format);
 ret = vsprintf(str, format, ap);
 va_end(ap);

 return ret;
}
