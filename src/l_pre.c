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
    struct prerequisite_entry_t *p;
    char *p_prerequisite;
#if DEBUG == 1
    unsigned i;
#endif  // DEBUG == 1

    if (!self || !prerequisite)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = malloc (sizeof (struct prerequisite_entry_t));
    p_prerequisite = strdup (prerequisite);

    if (!p || !p_prerequisite)
    {
        if (p)
            free (p);
        else
            _DBG_ ("Failed to allocate memory for %s.", "prerequisite entry");
        if (p_prerequisite)
            free (p_prerequisite);
        else
            _DBG_ ("Failed to allocate memory for %s.", "string");
        return true;
    }

    p->list_entry.next = NULL;
    p->prerequisite = p_prerequisite;

#if DEBUG == 1
    i = self->list.count;
#endif  // DEBUG == 1
    list_add_entry ((struct list_t *) self,  (struct list_entry_t *) p);

    _DBG_ ("Added new prerequisite #%u: '%s'", i, p->prerequisite);
    if (result)
        *result = p;
    return false;
}

bool
    prerequisites_print
    (
        struct prerequisites_t *self,
        FILE *stream
    )
{
    const struct prerequisite_entry_t *p;

    if (!self || !stream)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    for (p = (struct prerequisite_entry_t *) self->list.first; p;
         p = (struct prerequisite_entry_t *) p->list_entry.next)
        if (fprintf (stream, " %s", p->prerequisite) < 0)
        {
            _DBG ("Failed to write to output file.");
            return true;
        }

    return false;
}
