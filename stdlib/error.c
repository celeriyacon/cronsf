/*
 * error.c
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

#include <errno.h>
#include <string.h>

#undef NDEBUG
#include <assert.h>

int errno;

typedef struct _string_error_t_
{
 int errnum;
 const char* string;
} _string_error_t;

static _string_error_t error_strings[] =
{
 { 0, "Operation succeeded" },
 { EDOM, "EDOM" },
 { EILSEQ, "EILSEQ" },
 { ERANGE, "ERANGE" },
 { EACCES, "Permission denied" },
 { EAGAIN, "EAGAIN" },
 { EWOULDBLOCK, "EWOULDBLOCK" },
 { EBADF, "EBADF" },
 { EEXIST, "File exists" },
 { EINVAL, "Invalid argument" },
 { EIO, "EIO" },
 { EISDIR, "Is a directory" },
 { ENODEV, "ENODEV" },
 { ENOENT, "No such file or directory" },
 { ENOMEM, "Insufficient memory" },
 { ENOSPC, "No space left on device" },
 { ENOSYS, "Function not implemented" },
 { ENOTDIR, "Not a directory" },
 { EOVERFLOW, "EOVERFLOW" },
 { EPERM, "Operation not permitted" },
};

char *strerror(int errnum)
{
 if(errnum < 0 || errnum >= (sizeof(error_strings) / sizeof(error_strings[0])))
 {
  errno = EINVAL;
  return "Unknown error";
 }

 _string_error_t* p = &error_strings[errnum];

 assert(p->errnum == errnum);

 return (char*)p->string;
}

