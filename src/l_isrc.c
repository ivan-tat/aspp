/* l_isrc.c - input sources list structure.

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
#include "l_isrc.h"

bool
    input_sources_add
    (
        struct input_sources_t *self,
        const char *real,
        const char *base,
        const char *user,
        struct input_source_entry_t **result
    )
{
    struct input_source_entry_t *p;
    char *p_real, *p_base, *p_user;
#if DEBUG == 1
    unsigned i;
#endif  // DEBUG == 1

    if (!self || !real || !base || !user)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = malloc (sizeof (struct input_source_entry_t));
    p_real = strdup (real);
    p_base = strdup (base);
    p_user = strdup (user);

    if (!p || !p_real || !p_base || !p_user)
    {
        if (p)
            free (p);
        else
            _DBG_ ("Failed to allocate memory for %s.", "input source entry");
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

#if DEBUG == 1
    i = self->list.count;
#endif  // DEBUG == 1
    list_add_entry ((struct list_t *) self, (struct list_entry_t *) p);

    _DBG_
    (
        "Added new input source #%u:" NL
        "Input source #%u: user file = '%s'" NL
        "Input source #%u: base path = '%s'" NL
        "Input source #%u: real file = '%s'",
        i,
        i, p->user,
        i, p->base,
        i, p->real
    );

    if (result)
        *result = p;
    return false;
}

bool
    input_sources_find_real
    (
        struct input_sources_t *self,
        const char *real,
        struct input_source_entry_t **result
    )
{
    struct input_source_entry_t *p;
    unsigned i;

    if (!self || !real)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = (struct input_source_entry_t *) self->list.first;
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
        p = (struct input_source_entry_t *) p->list_entry.next;
        i++;
    }

    _DBG_ ("Failed to find real file '%s'.", real);
    if (result)
        *result = NULL;
    return true;
}

bool
    input_sources_find_user
    (
        struct input_sources_t *self,
        const char *user,
        struct input_source_entry_t **result
    )
{
    struct input_source_entry_t *p;
    unsigned i;

    if (!self || !user)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = (struct input_source_entry_t *) self->list.first;
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
        p = (struct input_source_entry_t *) p->list_entry.next;
        i++;
    }

    _DBG_ ("Failed to find user file '%s'.", user);
    if (result)
        *result = NULL;
    return true;
}

bool
    input_sources_add_with_check
    (
        struct input_sources_t *self,
        const char *user,
        const char *base_path_real,
        struct input_source_entry_t **result
    )
{
    char *tmp, *real;
    bool ok;
    unsigned len;

    tmp = NULL;
    real = NULL;
    ok = false;

    if (!self || !user || !base_path_real)
    {
        _DBG ("Bad arguments.");
        goto _local_exit;
    }

    if (check_path_abs (user))
    {
        // absolute path
        // first - check if it is already added
        real = resolve_full_path (user);
        if (!real)
        {
            // Fail
            _DBG_ ("Failed to resolve %s.", "real path");
            goto _local_exit;
        }
        if (input_sources_find_real (self, real, result))
        {
            if (input_sources_add (self, real, "", user, result))
            {
                // Fail
                goto _local_exit;
            }
        }
        // Success
        ok = true;
        //goto _local_exit;
    }
    else
    {
        // relative path
        // first - check if it is already added
        if (!input_sources_find_user (self, user, result))
        {
            // Success
            ok = true;
            //goto _local_exit;
        }
        else
        {
            len = strlen (base_path_real) + 1 + strlen (user) + 1;
            tmp = malloc (len);
            if (!tmp)
            {
                // Fail
                _DBG_ ("Failed to allocate memory for %s.", "string");
                goto _local_exit;
            }
            snprintf (tmp, len, "%s" PATHSEPSTR "%s", base_path_real, user);
            real = resolve_full_path (tmp);
            if (!real)
            {
                // Fail
                _DBG_ ("Failed to resolve %s.", "real path");
                goto _local_exit;
            }
            free (tmp);
            tmp = NULL; // to prevent free() on exit
            // trying real path - we are lucky
            if (check_file_exists (real))
            {
                if (!input_sources_add (self, real, base_path_real, user, result))
                {
                    // Success
                    ok = true;
                    //goto _local_exit;
                }
                // Fail
                //goto _local_exit;
            }
            else
            {
                // Fail
                _DBG_ ("Failed to find user file '%s'.", user);
                //goto _local_exit;
            }
        }
    }
_local_exit:
    if (tmp)
        free (tmp);
    if (real)
        free (real);
    if (!ok)
        if (result)
            *result = NULL;
    return !ok;
}

#if DEBUG == 1
void
    _DBG_input_sources_dump
    (
        struct input_sources_t *self
    )
{
    void *p;
    unsigned i;

    if (!self)
    {
        _DBG ("Bad arguments.");
        return;
    }

    p = (struct input_source_entry_t *) self->list.first;
    if (p)
    {
        i = 0;
        do
        {
            _DBG_
            (
                "Input source #%u: user file = '%s'" NL
                "Input source #%u: base path = '%s'" NL
                "Input source #%u: real file = '%s'",
                i, ((struct input_source_entry_t *) p)->user,
                i, ((struct input_source_entry_t *) p)->base,
                i, ((struct input_source_entry_t *) p)->real
            );
            p = (struct input_source_entry_t *) ((struct input_source_entry_t *) p)->list_entry.next;
            i++;
        }
        while (p);
    }
    else
        _DBG ("No input sources.");
}
#endif  // DEBUG == 1
