/* main.c - main program.

   aspp - simple assembler source file preprocessor.

   Author: Ivan Tatarinov, <ivan-tat@ya.ru>, 2019-2020.

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>.

   Home page: <https://gitlab.com/ivan-tat/aspp> */

#include "defs.h"

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <ctype.h>
#include "asmfile.h"
#include "debug.h"
#include "l_err.h"
#include "l_ifile.h"
#include "l_inc.h"
#include "l_isrc.h"
#include "l_list.h"
#include "l_pre.h"
#include "l_src.h"
#include "l_tgt.h"
#include "platform.h"

#define PROGRAM_NAME "aspp"

#define PROGRAM_VERSION "0.1" PROGRAM_VERSION_SUFFIX

#define PROGRAM_DESCRIPTION "Simple assembler source file preprocessor."

#define PROGRAM_LICENSE \
"License: public domain, <http://unlicense.org>" NL \
"This is free software; you are free to change and redistribute it." NL \
"There is NO WARRANTY, to the extent permitted by law."

#define PROGRAM_AUTHORS \
"Author: Ivan Tatarinov, <ivan-tat@ya.ru>, 2019-2020."

#define PROGRAM_CONTACTS \
"Home page: <https://gitlab.com/ivan-tat/aspp>"

#define HELP_HINT \
"Use '-h' or '--help' to get help."

// Acts

#define ACT_NONE       0
#define ACT_SHOW_HELP  1
#define ACT_PREPROCESS 2
#define ACT_MAKE_RULE  3

// Variables

struct errors_t
          errors           = { { .first = NULL, .last = NULL, .count = 0 } };
unsigned  v_act            = ACT_NONE;
char      v_act_show_help  = 0;
char      v_act_preprocess = 0;
char      v_act_make_rule  = 0;
char     *v_base_path_real = NULL;
struct include_paths_t
          v_include_paths  = { { .first = NULL, .last = NULL, .count = 0 } };
struct input_sources_t
          v_input_sources  = { { .first = NULL, .last = NULL, .count = 0 } };
struct sources_t
          v_sources        = { { .first = NULL, .last = NULL, .count = 0 } };
struct target_names_t
          v_target_names   = { { .first = NULL, .last = NULL, .count = 0 } };
char     *v_output_name;
struct prerequisites_t
          v_prerequisites  = { { .first = NULL, .last = NULL, .count = 0 } };

#if DEBUG == 1
void _DBG_dump_vars (void)
{
    _DBG_include_paths_dump (&v_include_paths);
    _DBG_input_sources_dump (&v_input_sources);
    _DBG_target_names_dump (&v_target_names);
    _DBG_ ("Output file name = '%s'", v_output_name);

}
#else   // DEBUG != 1
#define _DBG_dump_vars(x)
#endif  // DEBUG != 1

// Returns "false" on success.
bool add_error (const char *format, ...)
{
    va_list ap;
    bool status;

    va_start (ap, format);
    status = errors_add_vfmt (&errors, NULL, 1024, format, ap);
    va_end (ap);
    return status;
}

// Returns "false" on success.
bool add_missing_arg_error (const char *name, unsigned index)
{
    return add_error ("Missing parameter for %s (argument #%u).", name, index);
}

void show_errors (void)
{
    const struct error_entry_t *p;

    for (p = (struct error_entry_t *) errors.list.first; p;
         p = (struct error_entry_t *) p->list_entry.next)
        fprintf (stderr, "%s" NL, p->msg);
}

void exit_on_errors (void)
{
    if (errors.list.count)
    {
        fprintf (stderr, "Errors: %u. Stopped." NL, errors.list.count);
        exit (EXIT_FAILURE);
    }
}

void error_exit (const char *format, ...)
{
    va_list ap;

    va_start (ap, format);
    vfprintf (stderr, format, ap);
    va_end (ap);
    exit (EXIT_FAILURE);
}

void show_title (void)
{
    fprintf (stdout,
        "%s (version %s) - %s" NL
        "%s" NL
        "%s" NL
        "%s" NL,
        PROGRAM_NAME,
        PROGRAM_VERSION,
        PROGRAM_DESCRIPTION,
        PROGRAM_LICENSE,
        PROGRAM_AUTHORS,
        PROGRAM_CONTACTS
    );
}

void show_help (void)
{
    fprintf (stdout,
NL
"Usage:" NL
"    %s [options] [filename ...] [options]" NL
NL
"Options (GCC-compatible):" NL
"-h, --help      show this help and exit" NL
"-E              preprocess" NL
"-I <path>       include directory" NL
"-M[M]           output autodepend make rule" NL
"-MF <file>      autodepend output name" NL
"-MT <target>    autodepend target name (can be specified multiple times)" NL,
        PROGRAM_NAME
    );
}

char *_skip_blanks (char *s)
{
    if (s) while (*s != '\0' && isblank(*s)) s++;
    return s;
}

char *_skip_word (char *s)
{
    if (s)
    {
        if (*s == '_' || isalpha(*s))
        {
            s++;
            while (*s != '\0' && isalnum(*s)) s++;
        }
    }
    return s;
}

char *_skip_string (char *s)
{
    char *p;
    p = s;
    if (p)
    {
        if (*p == '"')
        {
            p++;        // Skip opening double quote character
            while (*p != '\0' && *p != '"') p++;
            if (*p == '"')
                p++;    // Skip closing double quote character
            else
                p = s;  // Fail
        }
    }
    return p;
}

// Result must be freed by caller.
char *_make_path (const char *a, const char *b)
{
    char *result;

    result = malloc (strlen (a) + 1 + strlen (b) + 1);  // including terminating zero
    if (result)
        sprintf (result, "%s" PATHSEPSTR "%s", a, b);
    else
        _DBG_ ("Failed to allocate memory for %s.", "string");
    return result;
}

// Returns "true" on success.
bool process_include (struct source_entry_t *src, char *f_loc, unsigned inc_flags)
{
    bool ok;
    char *tmp;
    char *src_base;
    char *src_base_tmp;
    char *inc_real, *inc_base, *inc_user;
    char *inc_real_tmp, *inc_base_tmp, *inc_user_tmp;
    char *inc_real_res;
    struct include_path_entry_t *resolved;

    ok = false;

    // free on exit:
    src_base_tmp = (char *) NULL;
    inc_real_tmp = (char *) NULL;
    inc_base_tmp = (char *) NULL;
    inc_user_tmp = (char *) NULL;
    inc_real_res = (char *) NULL;

    inc_user = f_loc;
    if (check_path_abs (f_loc))
    {
        // absolute source's path - use it as is
        _DBG_ ("Checking '%s'...", f_loc);
        inc_real = f_loc;
        inc_base_tmp = get_dir_name (f_loc);
        if (!inc_base_tmp)
        {
            // Fail
            _DBG_ ("Failed to allocate memory for %s.", "string");
            goto _local_exit;
        }
        inc_base = inc_base_tmp;
        if (!check_file_exists (f_loc))
            inc_flags &= ~SRCFL_PARSE;
        if (sources_add (&v_sources, inc_real, inc_base, inc_user, inc_flags, NULL))
        {
            // Fail
            goto _local_exit;
        }
        // Success
        ok = true;
    }
    else
    {
        // relative source's path - try to resolve real name
        src_base_tmp = get_dir_name (src->user);
        if (!src_base_tmp)
        {
            // Fail
            _DBG_ ("Failed to allocate memory for %s.", "string");
            goto _local_exit;
        }
        src_base = src_base_tmp;
        if (check_path_abs (src->user))
        {
            tmp = _make_path (src_base, src->user);
            if (!tmp)
            {
                // Fail
                goto _local_exit;
            }
            inc_real_tmp = resolve_full_path (tmp);
            free (tmp);
            if (!inc_real_tmp)
            {
                // Fail
                goto _local_exit;
            }
            inc_real = inc_real_tmp;
            inc_base = src_base;
        }
        else
        {
            tmp = get_dir_name (src->real);
            if (!tmp)
            {
                // Fail
                _DBG_ ("Failed to allocate memory for %s.", "string");
                goto _local_exit;
            }
            inc_real_tmp = _make_path (tmp, f_loc);
            free (tmp);
            if (!inc_real_tmp)
            {
                // Fail
                goto _local_exit;
            }
            tmp = inc_real_tmp;
            inc_real_tmp = resolve_full_path (tmp);
            free (tmp);
            if (!inc_real_tmp)
            {
                // Fail
                goto _local_exit;
            }
            inc_real = inc_real_tmp;
            inc_base = src->base;
            if (strcmp (src_base, ".") != 0)
            {
                tmp = _make_path (src_base, inc_user);
                if (!tmp)
                {
                    // Fail
                    goto _local_exit;
                }
                inc_user_tmp = resolve_full_path (tmp);
                free (tmp);
                if (!inc_user_tmp)
                {
                    // Fail
                    goto _local_exit;
                }
                inc_user = inc_user_tmp;
            }
        }
        if (check_file_exists (inc_real))
        {
            if (sources_add (&v_sources, inc_real, inc_base, inc_user, inc_flags, NULL))
            {
                // Fail
                goto _local_exit;
            }
            // Success
        }
        else
        {
            _DBG_ ("'%s' not found, resolving...", f_loc);
            if (include_paths_resolve_file (&v_include_paths, f_loc, &resolved))
            {
                tmp = _make_path (resolved->real, f_loc);
                if (!tmp)
                {
                    // Fail
                    goto _local_exit;
                }
                inc_real_res = resolve_full_path (tmp);
                free (tmp);
                if (!inc_real_res)
                {
                    // Fail
                    goto _local_exit;
                }
                inc_real = inc_real_res;
                inc_base = resolved->real;
                inc_user = f_loc;
            }
            else
            {
                inc_flags = 0;
            }
            if (sources_add (&v_sources, inc_real, inc_base, inc_user, 0, NULL))
            {
                // Fail
                goto _local_exit;
            }
            // Success
        }
        // Success
        ok = true;
    }
_local_exit:
    if (src_base_tmp)
        free (src_base_tmp);
    if (inc_real_tmp)
        free (inc_real_tmp);
    if (inc_base_tmp)
        free (inc_base_tmp);
    if (inc_user_tmp)
        free (inc_user_tmp);
    if (inc_real_res)
        free (inc_real_res);

    return ok;
}

#define TEXT_BUF_SIZE 15
// Returns "false" on success.
bool parse_source (struct source_entry_t *src)
{
    bool ok;
    struct asm_file_t file;
    char *t;
    char buf[TEXT_BUF_SIZE+1];
    char *s, *endp, *namep;
    unsigned tl, len, sl, namelen;
    char *f_loc, *f_loc_tmp;
    unsigned inc_flags;

    _DBG_ (
        "Source user file: '%s'" NL
        "Source base path: '%s'" NL
        "Source real file: '%s'",
        src->user, src->base, src->real);

    ok = false;

    // Free on exit (_local_exit):
    asm_file_clear (&file);
    t = (char *) NULL;

    if (!asm_file_load (&file, src->user))
    {
        // Fail
        goto _local_exit;
    }

    tl = 0;
    while (asm_file_next_line (&file, &s, &len))
    {
        // Free on exit (_loop_exit):
        f_loc_tmp = (char *) NULL;

        if (tl < len + 1)
        {
            tl = len + 1;       // + terminating zero
            if (t)
                free (t);
            t = malloc (tl);
            if (!t)
            {
                // Fail
                _DBG_ ("Failed to allocate memory for %s.", "string");
                goto _loop_exit;
            }
        }
        memcpy (t, s, len);
        t[len] = '\0';

        // RegEx pattern: [[:space:]]+(include|incbin)[[:space:]]+"[^[:space:]]+"
        // The rest of a line is not analized.
        s = t;

        // blanks
        endp = _skip_blanks (s);
        if (s == endp || *endp == '\0')
            goto _skip_line;
        // assembler directive
        s = endp;
        endp = _skip_word (s);
        if (s == endp || *endp == '\0')
            goto _skip_line;
        // (save it)
        sl = endp - s;
        if (sl > TEXT_BUF_SIZE)
            sl = TEXT_BUF_SIZE;
        memcpy (buf, s, sl);
        buf[sl] = '\0';
        // (check it)
        if (!strcasecmp (buf, "incbin"))
            inc_flags = 0;
        else if (!strcasecmp (buf, "include"))
            inc_flags = SRCFL_PARSE;
        else
            goto _skip_line;
        // blanks
        s = endp;
        endp = _skip_blanks (s);
        if (s == endp || *endp == '\0')
            goto _skip_line;
        // string
        s = endp;
        endp = _skip_string (s);
        if (s == endp)
            goto _skip_line;
        // (save it)
        namep = s + 1;          // skip opening double quotes character
        namelen = endp - s - 2; // excluding double quotes characters
        f_loc_tmp = malloc (namelen + 1);       // including terminating zero
        if (!f_loc_tmp)
        {
            // Fail
            _DBG_ ("Failed to allocate memory for %s.", "string");
            goto _loop_exit;
        }
        memcpy (f_loc_tmp, namep, namelen);
        f_loc_tmp[namelen] = '\0';
        f_loc = f_loc_tmp;
        // (done)

        if (!process_include (src, f_loc, inc_flags))
        {
            // Fail
            goto _loop_exit;
        }

        if (f_loc_tmp)
            free (f_loc_tmp);
        f_loc_tmp = (char *) NULL;
    _skip_line:;
    }

    ok = true;
    goto _local_exit;

_loop_exit:
    if (f_loc_tmp)
        free (f_loc_tmp);
_local_exit:
    asm_file_free (&file);
    if (t)
        free (t);
    return !ok;
}
#undef TEXT_BUF_SIZE

// Returns "false" on success.
bool make_rule (void)
{
    struct input_source_entry_t *isrc;
    struct source_entry_t *src, *last;

    for (isrc = (struct input_source_entry_t *) v_input_sources.list.first; isrc;
         isrc = (struct input_source_entry_t *) isrc->list_entry.next)
    {
        if (sources_add (&v_sources, isrc->real, isrc->base, isrc->user, SRCFL_PARSE, NULL))
            return true;        // Fail
    }

    if (v_sources.list.count)
    {
        src  = (struct source_entry_t *) v_sources.list.first;
        last = (struct source_entry_t *) v_sources.list.last;
        while (src != (struct source_entry_t *) last->list_entry.next)
        {
            while (src != (struct source_entry_t *) last->list_entry.next)
            {
                if (src->flags & SRCFL_PARSE)
                {
                    if (!parse_source (src))
                    {
                        if (prerequisites_add (&v_prerequisites, src->user, NULL))
                            return true;        // Fail
                    }
                    else
                    {
                        show_errors ();
                        exit_on_errors ();
                    }
                }
                else
                {
                    if (prerequisites_add (&v_prerequisites, src->user, NULL))
                            return true;        // Fail
                }
                src = (struct source_entry_t *) src->list_entry.next;
            }
            src = (struct source_entry_t *) last->list_entry.next;
            last = (struct source_entry_t *) v_sources.list.last;
        }
    }

    return false;       // Success
}

// Returns "false" on success.
bool write_rule (const char *name)
{
    FILE *f;

    f = fopen (name, "w");
    if (!f)
    {
        // Fail
        _DBG_ ("Failed to open file for %s.", "writing");
        return true;
    }

    if (target_names_print (&v_target_names, f))
        return true;    // Fail

    if (fprintf (f, ": ") < 0)
        return true;    // Fail

    if (prerequisites_print (&v_prerequisites, f))
        return true;    // Fail

    if (fprintf (f, NL) < 0)
        return true;    // Fail

    fclose (f);

    return false;       // Success
}

int main (int argc, char **argv)
{
    unsigned i;

    setlocale (LC_CTYPE, "C");

    if (argc == 1)
        error_exit ("No parameters. %s" NL, HELP_HINT);

    v_base_path_real = get_current_dir ();
    if (!v_base_path_real)
        error_exit ("Failed to get current directory." NL);
    _DBG_ ("Base path = '%s'", v_base_path_real);

    i = 1;
    while (i < argc)
    {
        if (strcmp (argv[i], "-h") == 0
        ||  strcmp (argv[i], "--help") == 0)
        {
            v_act_show_help = 1;
            i++;
        }
        else if (strcmp (argv[i], "-E") == 0)
        {
            v_act_preprocess = 1;
            i++;
        }
        else if (strcmp (argv[i], "-I") == 0)
        {
            i++;
            if (i == argc)
            {
                if (add_missing_arg_error ("-I", i))
                    exit (EXIT_FAILURE);
                break;
            }
            if (include_paths_add_with_check (&v_include_paths, argv[i], v_base_path_real, NULL))
                exit (EXIT_FAILURE);
            i++;
        }
        else if (strcmp (argv[i], "-M") == 0
             ||  strcmp (argv[i], "-MM") == 0)
        {
            v_act_make_rule = 1;
            i++;
        }
        else if (strcmp (argv[i], "-MF") == 0)
        {
            i++;
            if (i == argc)
            {
                if (add_missing_arg_error ("-MF", i))
                    exit (EXIT_FAILURE);
                break;
            }
            v_output_name = argv[i];
            i++;
        }
        else if (strcmp (argv[i], "-MT") == 0)
        {
            i++;
            if (i == argc)
            {
                if (add_missing_arg_error ("-MT", i))
                    exit (EXIT_FAILURE);
                break;
            }
            if (target_names_add (&v_target_names, argv[i], NULL))
                exit (EXIT_FAILURE);
            i++;
        }
        else if (argv[i][0] == '-')
        {
            if (add_error ("Unknown option '%s' (#%u).", argv[i], i))
                exit (EXIT_FAILURE);
            i++;
        }
        else
        {
            if (v_input_sources.list.count >= 1)
            {
                if (add_error ("Don't know what to do with input file '%s' (#%u).", argv[i], i))
                    exit (EXIT_FAILURE);
            }
            else
            {
                if (input_sources_add_with_check (&v_input_sources, argv[i], v_base_path_real, NULL))
                    error_exit ("Input source file '%s' was not found." NL, argv[i]);
            }
            i++;
        }
    }

    if (v_act_show_help)
    {
        if (v_act_preprocess + v_act_make_rule + v_include_paths.list.count + v_sources.list.count)
        {
            if (add_error ("Other arguments were ignored."))
                exit (EXIT_FAILURE);
        }
        v_act = ACT_SHOW_HELP;
    }
    else
    {
        if (v_act_preprocess + v_act_make_rule != 2)
        {
            if (add_error ("The only supported mode is when both options -E and -M are specified."))
                exit (EXIT_FAILURE);
        }
        v_act = ACT_MAKE_RULE;
    }

    if (errors.list.count)
    {
        if (v_act_show_help)
            show_title ();
        show_errors ();
        if (v_act_show_help)
        {
            show_help ();
            exit (EXIT_SUCCESS);
        }
        else
            exit_on_errors ();
    }

    switch (v_act)
    {
    case ACT_SHOW_HELP:
        show_title ();
        show_help ();
        break;
    case ACT_MAKE_RULE:
        if (!v_target_names.list.count)
        {
            if (add_error ("No target name was specified."))
                exit (EXIT_FAILURE);
        }
        if (!v_output_name || !strcmp (v_output_name, ""))
        {
            if (add_error ("No output name was specified."))
                exit (EXIT_FAILURE);
        }
        if (!v_input_sources.list.count)
        {
            if (add_error ("No source files were specified."))
                exit (EXIT_FAILURE);
        }
        if (errors.list.count)
        {
            show_errors ();
            exit_on_errors ();
        }
        if (!v_include_paths.list.count)
        {
            if (include_paths_add_with_check (&v_include_paths, ".", v_base_path_real, NULL))
                exit (EXIT_FAILURE);
        }
        _DBG_dump_vars ();
        if (make_rule ())
            error_exit ("Failed to parse sources.");
        if (write_rule (v_output_name))
            error_exit ("Failed to write to output file.");
        break;
    default:
        error_exit ("Action %u is not implemented yet.", v_act);
        break;
    }

    return EXIT_SUCCESS;
}
