#ifndef _STDLIB_ERRNO_H
#define _STDLIB_ERRNO_H

extern int errno;

#define EDOM		 1	// C
#define EILSEQ		 2	// C
#define ERANGE		 3	// C
#define EACCES		 4
#define EAGAIN		 5
#define EWOULDBLOCK	 6
#define EBADF		 7
#define EEXIST		 8
#define EINVAL		 9
#define EIO		10
#define EISDIR		11
#define ENODEV		12
#define ENOENT		13
#define ENOMEM		14
#define ENOSPC		15
#define ENOSYS		16
#define ENOTDIR		17
#define EOVERFLOW	18
#define EPERM		19

#endif
