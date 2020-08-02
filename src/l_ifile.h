/* l_ifile.h - declarations for "l_ifile.c".

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#ifndef _L_IFILE_H_INCLUDED
#define _L_IFILE_H_INCLUDED

#include "defs.h"

#include <stdbool.h>
#include <stdio.h>
#include "l_list.h"

// Include files list structure

#define IFILEFL_PARSE (1 << 0)

struct include_file_entry_t
{
    struct list_entry_t list_entry;
    unsigned line;
    unsigned flags;
    char *name;
};

struct include_files_t
{
    struct list_t list;
};

bool
    include_files_add
    (
        struct include_files_t *self,
        unsigned line,
        unsigned flags,
        const char *name,
        struct include_file_entry_t **result
    );

void include_files_free
    (
        struct include_files_t *self
    );

#endif  // !_L_IFILE_H_INCLUDED
