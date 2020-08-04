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
    bool ok;
    struct include_file_entry_t *p;
    char *p_name;
#if DEBUG == 1
    unsigned i;
#endif  // DEBUG == 1

    ok = false;
    p = (struct include_file_entry_t *) NULL;
    p_name = (char *) NULL;

    if (!self || !name)
    {
        _DBG ("Bad arguments.");
        goto _local_exit;
    }

    p = malloc (sizeof (struct include_file_entry_t));
    if (!p)
    {
        _perror ("malloc");
        goto _local_exit;
    }
    p_name = strdup (name);
    if (!p_name)
    {
        _perror ("strdup");
        goto _local_exit;
    }

    p->list_entry.next = NULL;
    p->line = line;
    p->flags = flags;
    p->name = p_name;

#if DEBUG == 1
    i = self->list.count;
#endif  // DEBUG == 1
    list_add_entry ((struct list_t *) self, (struct list_entry_t *) p);

    _DBG_ ("Added new include file #%u:", i);
    _DBG_ ("Include file #%u line = %u", i, p->line);
    _DBG_ ("Include file #%u flags = 0x%X", i, p->flags);
    _DBG_ ("Include file #%u user file = '%s'", i, p->name);

    ok = true;

_local_exit:
    if (!ok)
    {
        if (p)
            free (p);
        p = (struct include_file_entry_t *) NULL;
        if (p_name)
            free (p_name);
    }
    if (result)
        *result = p;
    return !ok;
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
