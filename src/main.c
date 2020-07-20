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
#include "debug.h"
#include "l_err.h"
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

// Returns "false" on success.
bool
    add_include_path
    (
        const char *real,
        const char *base,
        const char *user,
        struct include_path_entry_t **result
    )
{
    return include_paths_add (&v_include_paths, real, base, user, result);
}

// Returns "false" on success.
bool
    add_target_name
    (
        const char *user,
        struct target_name_entry_t **result
    )
{
    return target_names_add (&v_target_names, user, result);
}

// Returns "false" on success.
bool
    find_include_path_real
    (
        const char *real,
        struct include_path_entry_t **result
    )
{
    return include_paths_find_real (&v_include_paths, real, result);
}

// Returns "false" on success.
bool
    find_include_path_user
    (
        const char *user,
        struct include_path_entry_t **result
    )
{
    return include_paths_find_user (&v_include_paths, user, result);
}

// Returns "false" on success.
bool
    add_include_dir_with_check
    (
        const char *user,
        struct include_path_entry_t **result
    )
{
    return include_paths_add_with_check (&v_include_paths, user, v_base_path_real, result);
}

// Returns "false" on success.
bool
    resolve_file
    (
        const char *user,
        struct include_path_entry_t **result
    )
{
    return include_paths_resolve_file (&v_include_paths, user, result);
}

// Returns "false" on success.
bool
    add_input_source
    (
        const char *real,
        const char *base,
        const char *user,
        struct input_source_entry_t **result
    )
{
    return input_sources_add (&v_input_sources, real, base, user, result);
}

// Returns "false" on success.
bool
    find_input_source_real
    (
        const char *real,
        struct input_source_entry_t **result
    )
{
    return input_sources_find_real (&v_input_sources, real, result);
}

// Returns "false" on success.
bool
    find_input_source_user
    (
        const char *user,
        struct input_source_entry_t **result
    )
{
    return input_sources_find_user (&v_input_sources, user, result);
}

// Returns "false" on success.
bool
    add_input_source_with_check
    (
        const char *user,
        struct input_source_entry_t **result
    )
{
    return input_sources_add_with_check (&v_input_sources, user, v_base_path_real, result);
}

// Returns "false" on success.
bool
    add_source
    (
        const char *real,
        const char *base,
        const char *user,
        unsigned flags,
        struct source_entry_t **result
    )
{
    return sources_add (&v_sources, real, base, user, flags, result);
}

// Returns "false" on success.
bool
    find_source_real
    (
        const char *real,
        struct source_entry_t **result
    )
{
    return sources_find_real (&v_sources, real, result);
}

// Returns "false" on success.
bool
    find_source_user
    (
        const char *user,
        struct source_entry_t **result
    )
{
    return sources_find_user (&v_sources, user, result);
}

// Returns "false" on success.
bool
    add_prerequisite
    (
        const char *prerequisite,
        struct prerequisite_entry_t **result
    )
{
    return prerequisites_add (&v_prerequisites, prerequisite, result);
}

// Returns "false" on success.
bool print_prerequisites (FILE *stream)
{
    return prerequisites_print (&v_prerequisites, stream);
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

int main (int argc, char **argv)
{
    unsigned i;

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
            if (add_include_dir_with_check (argv[i], NULL))
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
            if (add_target_name (argv[i], NULL))
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
                if (add_input_source_with_check (argv[i], NULL))
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
        exit (EXIT_SUCCESS);
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
            if (add_include_dir_with_check (".", NULL))
                exit (EXIT_FAILURE);
        }
        _DBG_dump_vars ();
//        make_rule ();
//        write_rule (v_output_name);
        break;
    default:
        error_exit ("Action %u is not implemented yet.", v_act);
        break;
    }

    return EXIT_FAILURE;
}
