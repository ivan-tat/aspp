/* debug.h - declarations for "debug.c".

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#ifndef _DEBUG_H_INCLUDED
#define _DEBUG_H_INCLUDED

#include <stdarg.h>

void _DEBUG (const char *file, int line, const char *func, const char *format, ...);

#if DEBUG == 1
 #define _DBG(text)         _DEBUG (__FILE__, __LINE__, __func__, "%s", text)
 #define _DBG_(format, ...) _DEBUG (__FILE__, __LINE__, __func__, format, __VA_ARGS__)
#else   /* DEBUG != 1 */
 #define _DBG(text)
 #define _DBG_(format, ...)
#endif  /* DEBUG != 1 */

#endif  /* !_DEBUG_H_INCLUDED */
