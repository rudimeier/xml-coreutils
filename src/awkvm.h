/* 
 * Copyright (C) 2010 Laird Breyer
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 * 
 * Author:   Laird Breyer <laird@lbreyer.com>
 */

#ifndef AWKPARSER_H
#define AWKPARSER_H

#include "common.h"
#include "myerror.h"
#include "objstack.h"
#include "symbols.h"

#include "awkmem.h"

typedef struct {
  const char_t *pattern;
  int codestart;
} codeblock_t;

bool_t create_codeblock(codeblock_t *bl);
bool_t free_codeblock(codeblock_t *bl);
bool_t set_pattern_codeblock(codeblock_t *bl, const char *pattern);


typedef struct {
  int ic; /* instruction counter */
  flag_t flags;
  
  /* awkast_t *ast; */
  symbols_t sym;
  objstack_t symtable;

  awkconstmgr_t constants;
  objstack_t codeblocks;
} awkvm_t;

bool_t create_awkvm(awkvm_t *vm);
bool_t free_awkvm(awkvm_t *vm);

symbol_t getid_awkvm(awkvm_t *vm,  const char_t *begin, const char_t *end);
bool_t putsym_awkvm(awkvm_t *vm, symbol_t id);
symrec_t *getsym_awkvm(awkvm_t *vm, symbol_t id);

/* see awkp.y */
bool_t parse_string_awkvm(awkvm_t *vm, const char_t *begin, const char_t *end);
bool_t parse_file_awkvm(awkvm_t *vm, const char *file);

/* see awkl.l */
void reset_lexer_awk();
void scan_lexer_awk(char *buf, int len);
void free_lexer_awk();


#include "awkp.h" /* bison generates this */

#endif
