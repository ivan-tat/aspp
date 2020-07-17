/* l_pre.h - declarations for "l_pre.c".

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#ifndef _L_PRE_H_INCLUDED
#define _L_PRE_H_INCLUDED

#include "defs.h"

#include <stdbool.h>
#include "l_list.h"

// Prerequisites list structure

struct prerequisite_entry_t
{
    struct list_entry_t list_entry;
    char *prerequisite;
};

struct prerequisites_t
{
    struct list_t list;
};

// Returns "false" on success ("result" if presents is set to list entry).
bool
    prerequisites_add
    (
        struct prerequisites_t *self,
        const char *prerequisite,
        struct prerequisite_entry_t **result
    );

// Returns "false" on success.
bool
    prerequisites_print
    (
        struct prerequisites_t *self,
        FILE *stream
    );

#endif  // !_L_PRE_H_INCLUDED
