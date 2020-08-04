/* l_pre.c - prerequisites list structure.

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#include "defs.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "l_list.h"
#include "l_pre.h"

bool
    prerequisites_add
    (
        struct prerequisites_t *self,
        const char *prerequisite,
        struct prerequisite_entry_t **result
    )
{
    bool ok;
    struct prerequisite_entry_t *p;
    char *p_prerequisite;
#if DEBUG == 1
    unsigned i;
#endif  // DEBUG == 1

    ok = false;
    p = (struct prerequisite_entry_t *) NULL;
    p_prerequisite = (char *) NULL;

    if (!self || !prerequisite)
    {
        _DBG ("Bad arguments.");
        goto _local_exit;
    }

    p = malloc (sizeof (struct prerequisite_entry_t));
    if (!p)
    {
        _perror ("malloc");
        goto _local_exit;
    }
    p_prerequisite = strdup (prerequisite);
    if (!p_prerequisite)
    {
        _perror ("strdup");
        goto _local_exit;
    }

    p->list_entry.next = NULL;
    p->prerequisite = p_prerequisite;

#if DEBUG == 1
    i = self->list.count;
#endif  // DEBUG == 1
    list_add_entry ((struct list_t *) self,  (struct list_entry_t *) p);

    _DBG_ ("Added new prerequisite #%u: '%s'", i, p->prerequisite);

    ok = true;

_local_exit:
    if (!ok)
    {
        if (p)
        {
            free (p);
            p = (struct prerequisite_entry_t *) NULL;
        }
        if (p_prerequisite)
            free (p_prerequisite);
    }
    if (result)
        *result = p;
    return !ok;
}

bool
    prerequisites_print
    (
        struct prerequisites_t *self,
        FILE *stream
    )
{
    const struct prerequisite_entry_t *p;
    bool padding;

    if (!self || !stream)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    padding = false;
    for (p = (struct prerequisite_entry_t *) self->list.first; p;
         p = (struct prerequisite_entry_t *) p->list_entry.next)
    {
        if (fprintf (stream, padding ? " %s" : "%s", p->prerequisite) < 0)
        {
            _perror ("fprintf");
            return true;
        }
        padding = true;
    }

    return false;
}
