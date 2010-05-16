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

#include "awkmem.h"
#include "mem.h"
#include "math.h"

#include <string.h>

bool_t create_awkmem(awkmem_t *am) {
  if( am ) {
    am->brk = 0;
    return create_mem(&am->start, &am->size, sizeof(byte_t), 1024);
  }
  return FALSE;
}

bool_t free_awkmem(awkmem_t *am) {
  if( am ) {
    am->brk = 0;
    return free_mem(&am->start, &am->size);
  }
  return FALSE;
}

bool_t reset_awkmem(awkmem_t *am, awkmem_ptr_t p) {
  if( am && (p < am->size) ) {
    am->brk = p;
    return TRUE;
  }
  return FALSE;
}

awkmem_ptr_t sbrk_awkmem(awkmem_t *am, size_t numbytes) {
  if( am ) {
    if( numbytes == 0 ) {
      return am->brk;
    } else if( ensure_bytes_mem(numbytes + am->brk, 
				&am->start, &am->size, sizeof(byte_t)) ) {
      am->brk += numbytes * sizeof(byte_t);
      return (am->brk - numbytes * sizeof(byte_t));
    }
  }
  return (awkmem_ptr_t)(-1);
}

/***/

bool_t create_awkstrings(awkstrings_t *as) {
  if( as ) {
    return create_awkmem(&as->rom);
  }
  return FALSE;
}

bool_t free_awkstrings(awkstrings_t *as) {
  if( as ) {
    return free_awkmem(&as->rom);
  }
  return FALSE;
}

/* insert both a string and its floating point value into as->rom */
awkmem_ptr_t insert_awkstrings(awkstrings_t *as, const char_t *begin, const char_t *end) {
  awkmem_ptr_t p = (awkmem_ptr_t)(-1);
  awknum_t v;
  if( as && begin && (end >= begin) ) {
    p = sbrk_awkmem(&as->rom, end - begin + 1 + sizeof(awknum_t));
    if( p != (awkmem_ptr_t)(-1) ) {
      memcpy(as->rom.start + p + sizeof(awknum_t), begin, end - begin);
      as->rom.start[p + sizeof(awknum_t) + end - begin] = '\0';
      v = atof((char *)(as->rom.start + p + sizeof(awknum_t)));
      memcpy(as->rom.start + p, &v, sizeof(awknum_t));
    }
  }
  return p;
}

awkstring_t string_awkstrings(awkstrings_t *as, awkmem_ptr_t p) {
  return (as && (p < as->rom.size)) ? 
    (awkstring_t)(as->rom.start + p + sizeof(awknum_t)) : NULL;
}

awknum_t number_awkstrings(awkstrings_t *as, awkmem_ptr_t p) {
  return (as && (p < as->rom.size)) ? *(awknum_t *)(as->rom.start + p) : NAN;
}

