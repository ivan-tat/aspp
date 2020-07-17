/* l_list.c - basic list structure.

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#include "defs.h"

#include "l_list.h"

void
    list_add_entry
    (
        struct list_t *self,
        struct list_entry_t *p
    )
{
    if (self->first)
    {
        self->last->next = p;
        self->last = p;
    }
    else
    {
        self->first = p;
        self->last = p;
    }

    self->count++;
}
