/* l_tgt.c - target names list structure.

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#include "defs.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "l_list.h"
#include "l_tgt.h"

bool
    target_names_add
    (
        struct target_names_t *self,
        const char *name,
        struct target_name_entry_t **result
    )
{
    struct target_name_entry_t *p;
    char *p_name;
#if DEBUG == 1
    unsigned i;
#endif  // DEBUG == 1

    if (!self || !name)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = malloc (sizeof (struct target_name_entry_t));
    p_name = strdup (name);

    if (!p || !p_name)
    {
        if (p)
            free (p);
        else
            _DBG_ ("Failed to allocate memory for %s.", "target name entry");
        if (p_name)
            free (p_name);
        else
            _DBG_ ("Failed to allocate memory for %s.", "string");
        return true;
    }

    p->list_entry.next = NULL;
    p->name = p_name;

#if DEBUG == 1
    i = self->list.count;
#endif  // DEBUG == 1
    list_add_entry ((struct list_t *) self, (struct list_entry_t *) p);

    _DBG_ ("Added new target name #%u: '%s'", i, p->name);

    if (result)
        *result = p;
    return false;
}

bool
    target_names_print
    (
        struct target_names_t *self,
        FILE *stream
    )
{
    const struct target_name_entry_t *p;
    bool f;

    if (!self || !stream)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    f = false;
    for (p = (struct target_name_entry_t *) self->list.first; p;
         p = (struct target_name_entry_t *) p->list_entry.next)
    {
        if (fprintf (stream, f ? " %s" : "%s", p->name) < 0)
        {
            _DBG ("Failed to write to output file.");
            return true;
        }
        f = true;
    }

    return false;
}

#if DEBUG == 1
void
    _DBG_target_names_dump
    (
        struct target_names_t *self
    )
{
    struct target_name_entry_t *p;
    unsigned i;

    if (!self)
    {
        _DBG ("Bad arguments.");
        return;
    }

    p = (struct target_name_entry_t *) self->list.first;
    if (p)
    {
        i = 0;
        do
        {
            _DBG_
            (
                "Target #%u: user file '%s'",
                i, ((struct target_name_entry_t *) p)->name
            );
            p = (struct target_name_entry_t *) ((struct target_name_entry_t *) p)->list_entry.next;
            i++;
        }
        while (p);
    }
    else
        _DBG ("No target names.");
}
#endif  // DEBUG == 1
