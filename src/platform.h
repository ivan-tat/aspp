/* platform.h - declarations for "platform.c".

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#ifndef _PLATFORM_H_INCLUDED
#define _PLATFORM_H_INCLUDED

#include "defs.h"

#include <stdbool.h>

// Returns "true" on success.
bool check_path_abs (const char *path);

// Returns "true" on success.
bool check_path_exists (const char *path);

// Returns "true" on success.
bool check_file_exists (const char *path);

#endif  // !_PLATFORM_H_INCLUDED
