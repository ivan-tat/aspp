/* l_src.c - sources list structure.

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
#include "l_src.h"

bool
    sources_add
    (
        struct sources_t *self,
        const char *real,
        const char *base,
        const char *user,
        unsigned flags,
        struct source_entry_t **result
    )
{
    struct source_entry_t *p;
    char *p_real, *p_base, *p_user;
#if DEBUG == 1
    unsigned i;
#endif  // DEBUG == 1

    if (!self || !real || !base || !user)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = malloc (sizeof (struct source_entry_t));
    p_real = strdup (real);
    p_base = strdup (base);
    p_user = strdup (user);

    if (!p || !p_real || !p_base || !p_user)
    {
        if (p)
            free (p);
        else
            _DBG_ ("Failed to allocate memory for %s.", "source entry");
        if (p_real)
            free (p_real);
        else
            _DBG_ ("Failed to allocate memory for %s.", "real file string");
        if (p_base)
            free (p_base);
        else
            _DBG_ ("Failed to allocate memory for %s.", "base path string");
        if (p_user)
            free (p_user);
        else
            _DBG_ ("Failed to allocate memory for %s.", "user file string");
        return true;
    }

    p->list_entry.next = NULL;
    p->real = p_real;
    p->base = p_base;
    p->user = p_user;
    p->flags = flags;

#if DEBUG == 1
    i = self->list.count;
#endif  // DEBUG == 1
    list_add_entry ((struct list_t *) self, (struct list_entry_t *) p);

    _DBG_
    (
        "Added new source #%u:" NL
        "Source #%u: flags     = 0x%x" NL
        "Source #%u: user file = '%s'" NL
        "Source #%u: base path = '%s'" NL
        "Source #%u: real file = '%s'",
        i,
        i, p->flags,
        i, p->user,
        i, p->base,
        i, p->real
    );

    if (result)
        *result = p;
    return false;
}

bool
    sources_find_real
    (
        struct sources_t *self,
        const char *real,
        struct source_entry_t **result
    )
{
    struct source_entry_t *p;
    unsigned i;

    if (!self || !real)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = (struct source_entry_t *) self->list.first;
    i = 0;
    while (p)
    {
        if (!strcmp (p->real, real))
        {
            _DBG_ ("Found user file '%s' (real file '%s') at #%u.", p->user, p->real, i);
            if (result)
                *result = p;
            return false;
        }
        p = (struct source_entry_t *) p->list_entry.next;
        i++;
    }

    _DBG_ ("Failed to find real file '%s'.", real);
    if (result)
        *result = NULL;
    return true;
}

bool
    sources_find_user
    (
        struct sources_t *self,
        const char *user,
        struct source_entry_t **result
    )
{
    struct source_entry_t *p;
    unsigned i;

    if (!self || !user)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = (struct source_entry_t *) self->list.first;
    i = 0;
    while (p)
    {
        if (!strcmp (p->user, user))
        {
            _DBG_ ("Found user file '%s' (real file '%s') at #%u.", p->user, p->real, i);
            if (result)
                *result = p;
            return false;
        }
        p = (struct source_entry_t *) p->list_entry.next;
        i++;
    }

    _DBG_ ("Failed to find user file '%s'.", user);
    if (result)
        *result = NULL;
    return true;
}
