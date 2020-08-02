/* l_ifile.c -- include files list structure.

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#include "defs.h"

#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "platform.h"
#include "l_list.h"
#include "l_ifile.h"

bool
    include_files_add
    (
        struct include_files_t *self,
        unsigned line,
        unsigned flags,
        const char *name,
        struct include_file_entry_t **result
    )
{
    struct include_file_entry_t *p;
    char *p_name;
#if DEBUG == 1
    unsigned i;
#endif  // DEBUG == 1

    if (!self || !name)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = malloc (sizeof (struct include_file_entry_t));
    p_name = strdup (name);

    if (!p || !p_name)
    {
        if (p)
            free (p);
        else
            _DBG_ ("Failed to allocate memory for %s.", "include file entry");
        if (p_name)
            free (p_name);
        else
            _DBG_ ("Failed to allocate memory for %s.", "string");
        return true;
    }

    p->list_entry.next = NULL;
    p->line = line;
    p->flags = flags;
    p->name = p_name;

#if DEBUG == 1
    i = self->list.count;
#endif  // DEBUG == 1
    list_add_entry ((struct list_t *) self, (struct list_entry_t *) p);

    _DBG_
    (
        "Added new include file #%u:" NL
        "Include file #%u: line = %u" NL
        "Include file #%u: flags = 0x%X" NL
        "Include file #%u: user file = '%s'",
        i,
        i, p->line,
        i, p->flags,
        i, p->name
    );

    if (result)
        *result = p;
    return false;
}

void include_files_free
    (
        struct include_files_t *self
    )
{
    struct include_file_entry_t *p, *n;

    p = (struct include_file_entry_t *) self->list.first;
    while (p)
    {
        if (p->name)
            free (p->name);
        n = (struct include_file_entry_t *) p->list_entry.next;
        free (p);
        p = n;
    }
    self->list.first = NULL;
    self->list.last = NULL;
    self->list.count = 0;
}
