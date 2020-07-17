/* l_list.h - declarations for "l_list.c".

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#ifndef _L_LIST_H_INCLUDED
#define _L_LIST_H_INCLUDED

#include "defs.h"

// List structure

struct list_entry_t
{
    struct list_entry_t *next;
};

struct list_t
{
    struct list_entry_t *first, *last;
    unsigned count;
};

void
    list_add_entry
    (
        struct list_t *self,
        struct list_entry_t *p
    );

#endif  // !_L_LIST_H_INCLUDED
