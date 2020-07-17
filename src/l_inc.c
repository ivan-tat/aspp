//
// l_inc.c -- include paths list structure.
//
// This is free and unencumbered software released into the public domain.
// For more information, please refer to <http://unlicense.org>.
//

#include "defs.h"

#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "platform.h"
#include "l_list.h"
#include "l_inc.h"

bool
    include_paths_add
    (
        struct include_paths_t *self,
        const char *real,
        const char *base,
        const char *user,
        struct include_path_entry_t **result
    )
{
    struct include_path_entry_t *p;
    char *p_real, *p_base, *p_user;
#if DEBUG == 1
    unsigned i;
#endif  // DEBUG == 1

    if (!self || !real || !base || !user)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = malloc (sizeof (struct include_path_entry_t));
    p_real = strdup (real);
    p_base = strdup (base);
    p_user = strdup (user);

    if (!p || !p_real || !p_base || !p_user)
    {
        if (p)
            free (p);
        else
            _DBG_ ("Failed to allocate memory for %s.", "include path entry");
        if (p_real)
            free (p_real);
        else
            _DBG_ ("Failed to allocate memory for %s.", "real path string");
        if (p_base)
            free (p_base);
        else
            _DBG_ ("Failed to allocate memory for %s.", "base path string");
        if (p_user)
            free (p_user);
        else
            _DBG_ ("Failed to allocate memory for %s.", "user path string");
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
        "Added new include path #%u:" NL
        "Include path #%u: user path = '%s'" NL
        "Include path #%u: base path = '%s'" NL
        "Include path #%u: real path = '%s'",
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
    include_paths_find_real
    (
        struct include_paths_t *self,
        const char *real,
        struct include_path_entry_t **result
    )
{
    struct include_path_entry_t *p;
    unsigned i;

    if (!self || !real)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = (struct include_path_entry_t *) self->list.first;
    i = 0;
    while (p)
    {
        if (!strcmp (p->real, real))
        {
            _DBG_ ("Found user path '%s' (real path '%s') at #%u.", p->user, p->real, i);
            if (result)
                *result = p;
            return false;
        }
        p = (struct include_path_entry_t *) p->list_entry.next;
        i++;
    }

    _DBG_ ("Failed to find real path '%s'.", real);
    if (result)
        *result = NULL;
    return true;
}

bool
    include_paths_find_user
    (
        struct include_paths_t *self,
        const char *user,
        struct include_path_entry_t **result
    )
{
    struct include_path_entry_t *p;
    unsigned i;

    if (!self || !user)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = (struct include_path_entry_t *) self->list.first;
    i = 0;
    while (p)
    {
        if (!strcmp (p->user, user))
        {
            _DBG_ ("Found user path '%s' (real path '%s') at #%u.", p->user, p->real, i);
            if (result)
                *result = p;
            return false;
        }
        p = (struct include_path_entry_t *) p->list_entry.next;
        i++;
    }

    _DBG_ ("Failed to find user path '%s'.", user);
    if (result)
        *result = NULL;
    return true;
}

bool
    include_paths_add_with_check
    (
        struct include_paths_t *self,
        const char *user,
        const char *base_path_real,
        struct include_path_entry_t **result
    )
{
    char *real;
    bool ok;

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
        real = (char *) user;
        if (include_paths_find_real (self, real, result))
        {
            if (include_paths_add (self, real, "", user, result))
            {
                // Fail
                real = NULL;    // to prevent free()
                goto _local_exit;
            }
        }
        // Success
        real = NULL;    // to prevent free()
        ok = true;
        //goto _local_exit;
    }
    else
    {
        // relative path
        if (include_paths_find_user (self, user, result))
        {
            if (strlen (base_path_real) + 1 + strlen (user) >= PATH_MAX)
            {
                // Fail
                _DBG ("Path is too large.");
                goto _local_exit;
            }
            real = malloc (PATH_MAX);
            if (!real)
            {
                // Fail
                _DBG_ ("Failed to allocate memory for %s.", "string");
                goto _local_exit;
            }
            snprintf (real, PATH_MAX, "%s/%s", base_path_real, user);
            if (check_path_exists (real))
            {
                if (!include_paths_add (self, real, base_path_real, user, result))
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
                _DBG_ ("Failed to find user path '%s'.", user);
                //goto _local_exit;
            }
        }
    }
_local_exit:
    if (real)
        free (real);
    if (!ok)
        if (result)
            *result = NULL;
    return !ok;
}

#if DEBUG == 1
void
    _DBG_include_paths_dump
    (
        struct include_paths_t *self
    )
{
    struct include_path_entry_t *p;
    unsigned i;

    if (!self)
    {
        _DBG ("Bad arguments.");
        return;
    }

    p = (struct include_path_entry_t *) self->list.first;
    if (p)
    {
        i = 0;
        do
        {
            _DBG_
            (
                "Include path #%u: user path = '%s'" NL
                "Include path #%u: base path = '%s'" NL
                "Include path #%u: real path = '%s'",
                i, ((struct include_path_entry_t *) p)->user,
                i, ((struct include_path_entry_t *) p)->base,
                i, ((struct include_path_entry_t *) p)->real
            );
            p = (struct include_path_entry_t *) ((struct include_path_entry_t *) p)->list_entry.next;
            i++;
        }
        while (p);
    }
    else
        _DBG ("No include paths.");
}
#endif  // DEBUG == 1