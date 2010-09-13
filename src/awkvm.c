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

#include "awkvm.h"
#include <string.h>

bool_t create_codeblock(codeblock_t *bl) {
  if( bl ) {
    memset(bl, 0, sizeof(codeblock_t));
    return TRUE;
  }
  return FALSE;
}

bool_t free_codeblock(codeblock_t *bl) {
  if( bl ) {
    return TRUE;
  }
  return FALSE;
}

bool_t set_pattern_codeblock(codeblock_t *bl, const char *pattern) {
  if( bl ) {
    return (pattern != NULL);
  }
  return FALSE;
}


bool_t create_awkvm(awkvm_t *vm) {
  bool_t ok = TRUE;
  if( vm ) {
    memset(vm, 0, sizeof(awkvm_t));
    ok &= create_symbols(&vm->sym);
    ok &= create_objstack(&vm->symtable, sizeof(symrec_t));
    ok &= create_awkconstmgr(&vm->constants);
    ok &= create_objstack(&vm->codeblocks, sizeof(codeblock_t));
    return ok;
  }
  return FALSE;
}


bool_t free_awkvm(awkvm_t *vm) {
  if( vm ) {
    free_objstack(&vm->codeblocks);
    free_awkconstmgr(&vm->constants);
    free_objstack(&vm->symtable);
    free_symbols(&vm->sym);
  }
  return FALSE;
}

symbol_t getid_awkvm(awkvm_t *vm,  const char_t *begin, const char_t *end) {
  return vm ? getvalue_symbols(&vm->sym, TRUE, begin, end) : -1;
}

bool_t putsym_awkvm(awkvm_t *vm, symbol_t id) {
  if( vm && (id == vm->sym.last) ) {
    return push_objstack(&vm->symtable, NULL, sizeof(symrec_t));
  }
  return FALSE;
}

symrec_t *getsym_awkvm(awkvm_t *vm, symbol_t id) {
  return vm ? get_objstack(&vm->symtable, id, sizeof(symrec_t)) : NULL;
}
