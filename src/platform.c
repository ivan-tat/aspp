/* platform.c - platform-specific library.

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#include "defs.h"

#include <limits.h>
#include <sys/stat.h>
#include "platform.h"

bool check_path_abs (const char *path)
{
    return (path && path[0] == '/');
}

bool check_path_exists (const char *path)
{
    struct stat st;
    if (!stat (path, &st))
        return false;
    return (S_ISDIR (st.st_mode)) != 0;
}

bool check_file_exists (const char *path)
{
    struct stat st;
    if (!stat (path, &st))
        return false;
    return (S_ISREG (st.st_mode)) || ((st.st_mode & S_IFMT) == 0);
}
