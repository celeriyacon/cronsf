#ifndef _STDLIB_STDBOOL_H
#define _STDLIB_STDBOOL_H

#ifndef __cplusplus
/*
 typedef _Bool bool;

 #define false ((bool)0)
 #define true ((bool)1)

 #define __bool_true_false_are_defined true
*/
 // More C vs C++ sizeof() incompatibilities, yay~
 #define bool _Bool

 #define false 0
 #define true 1
 #define __bool_true_false_are_defined 1

#endif

#endif
