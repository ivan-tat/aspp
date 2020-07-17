/* l_err.c - errors list structure.

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
#include "l_err.h"

bool
    errors_add
    (
        struct errors_t *self,
        const char *msg,
        struct error_entry_t **result
    )
{
    struct error_entry_t *p;
    char *p_msg;

    if (!self || !msg)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    p = malloc (sizeof (struct error_entry_t));
    p_msg = strdup (msg);

    if (!p || !p_msg)
    {
        if (p)
            free (p);
        else
            _DBG_ ("Failed to allocate memory for %s.", "error entry");
        if (p_msg)
            free (p_msg);
        else
            _DBG_ ("Failed to allocate memory for %s.", "string");
        return true;
    }

    p->list_entry.next = NULL;
    p->msg = p_msg;

    list_add_entry ((struct list_t *) self,  (struct list_entry_t *) p);

    if (result)
        *result = p;
    return false;
}

bool
    errors_add_vfmt
    (
        struct errors_t *self,
        struct error_entry_t **result,
        unsigned bufsize,
        const char *format,
        va_list ap
    )
{
    char *s;
    bool status;

    if (!self || !bufsize || !format)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    s = malloc (bufsize);
    if (!s)
    {
        _DBG_ ("Failed to allocate memory for %s.", "string buffer");
        return true;
    }
    vsnprintf (s, bufsize, format, ap);
    status = errors_add (self, s, result);
    free (s);
    return status;
}

bool
    errors_add_fmt
    (
        struct errors_t *self,
        struct error_entry_t **result,
        unsigned bufsize,
        const char *format,
        ...
    )
{
    va_list ap;
    bool status;

    va_start (ap, format);

    if (!self || !bufsize || !format)
    {
        _DBG ("Bad arguments.");
        return true;
    }

    status = errors_add_vfmt (self, result, bufsize, format, ap);
    va_end (ap);
    return status;
}
