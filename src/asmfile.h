/* asmfile.h - declarations for "asmfile.c".

   This is free and unencumbered software released into the public domain.
   For more information, please refer to <http://unlicense.org>. */

#ifndef _ASMFILE_H_INCLUDED
#define _ASMFILE_H_INCLUDED

#include "defs.h"

#include <stdbool.h>
#include "l_list.h"
//#include "l_ifile.h"

// Assembler file structure

struct asm_file_t
{
    char *data;
    long size;
    long pos;
    long line;
    long line_start;
    long line_end;
//    struct include_files_t includes;
};

void asm_file_clear (struct asm_file_t *self);

// Returns "true" on success.
bool asm_file_load (struct asm_file_t *self, const char *name);

void asm_file_free (struct asm_file_t *self);

// Returns "true" on success.
bool asm_file_eof (struct asm_file_t *self);

// Returns "true" on success.
bool asm_file_eol (struct asm_file_t *self);

// Returns "true" on success.
bool asm_file_next_line (struct asm_file_t *self, char **s, unsigned *len);

#endif  // !_ASMFILE_H_INCLUDED
