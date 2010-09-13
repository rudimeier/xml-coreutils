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

void *get_awkmem(awkmem_t *am, awkmem_ptr_t p) {
  return (am && (p >= 0) && (p <= am->brk)) ? (am + p) : NULL;
}

/***/

bool_t create_awkconstmgr(awkconstmgr_t *ac) {
  if( ac ) {
    return create_awkmem(&ac->rom);
  }
  return FALSE;
}

bool_t free_awkconstmgr(awkconstmgr_t *ac) {
  if( ac ) {
    return free_awkmem(&ac->rom);
  }
  return FALSE;
}

awkstring_t get_string_awkconstmgr(awkconstmgr_t *ac, awkmem_ptr_t p) {
  awkconst_t *c = (awkconst_t *)get_awkmem(&ac->rom, p);
  return c ? &c->strval : NULL;
}

awknum_t get_number_awkconstmgr(awkconstmgr_t *ac, awkmem_ptr_t p) {
  awkconst_t *c = (awkconst_t *)get_awkmem(&ac->rom, p);
  return c ? c->numval : NAN;
}

/* insert both a string and its floating point value into as->rom */
awkmem_ptr_t append_awkconstmgr(awkconstmgr_t *ac, const char_t *begin, const char_t *end) {
  awkmem_ptr_t p = (awkmem_ptr_t)(-1);
  awkconst_t *c;
  if( ac && begin && (end >= begin) ) {
    p = sbrk_awkmem(&ac->rom, end - begin + sizeof(awkconst_t));
    if( p != (awkmem_ptr_t)(-1) ) {
      c = (awkconst_t *)(ac->rom.start + p);
      memcpy(&c->strval, begin, end - begin);
      ac->rom.start[p + end - begin + sizeof(awkconst_t)] = '\0';
      c->numval = atof(&c->strval);
    }
  }
  return p;
}

/***/

bool_t create_awkvar(awkvar_t *v) {
  if( v ) {
    memset(v, 0, sizeof(awkvar_t));
  }
  return FALSE;
}

bool_t free_awkvar(awkvar_t *v) {
  if( v ) {
    memset(v, 0, sizeof(awkvar_t));
  }
  return FALSE;
}

