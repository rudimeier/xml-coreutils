/* 
 * Copyright (C) 2006 Laird Breyer
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#include "common.h"
#include "tempcollect.h"
#include "mem.h"
#include "error.h"
#include "stdout.h"
#include <string.h>

bool_t create_tempcollect(tempcollect_t *tc, const char_t *name,
			  size_t buflen, size_t maxbuflen) {
  if( tc ) {
    tc->max_buflen = maxbuflen;
    create_mem(&tc->buf, &tc->buflen, sizeof(byte_t), 128);
    tc->bufpos = 0;
    tc->name = name;
    return (tc->buf != NULL);
  }
  return FALSE;
}

bool_t free_tempcollect(tempcollect_t *tc) {
  if( tc ) {
    if( tc->buf ) {
      free_mem(&tc->buf, &tc->buflen);
    }
    return TRUE;
  }
  return FALSE;
}

bool_t reset_tempcollect(tempcollect_t *tc) {
  if( tc ) {
    tc->bufpos = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t write_tempcollect(tempcollect_t *tc, const byte_t *buf, size_t buflen) {
  if( tc && buf && (buflen > 0) ) {
    while( (tc->buflen < tc->max_buflen) &&
	   (tc->bufpos + buflen > tc->buflen) ) {
      if( !grow_mem(&tc->buf, &tc->buflen, sizeof(byte_t), 128) ) {
	break;
      }
    }
    if( tc->bufpos + buflen > tc->buflen ) {
      errormsg(E_WARNING, 
	       "maximum tempcollect buffer size reached, some data will be lost"); 
      buflen = tc->buflen - tc->bufpos;
    }
    memcpy(tc->buf + tc->bufpos, buf, buflen);
    tc->bufpos += buflen;
    return (bool_t)(buflen > 0);
  }
  return FALSE;
}

bool_t read_tempcollect(tempcollect_t *tc, byte_t *buf, size_t buflen) {
  if( tc && buf && (buflen > 0) ) {
    return TRUE;
  }
  return FALSE;
}

bool_t write_stdout_tempcollect(tempcollect_t *tc) {
  if( tc && tc->buf ) {
    return write_stdout(tc->buf, tc->buflen);
  }
  return FALSE;
}

bool_t squeeze_stdout_tempcollect(tempcollect_t *tc) {
  if( tc && tc->buf ) {
    return squeeze_stdout(tc->buf, tc->buflen);
  }
  return FALSE;
}

bool_t create_tclist(tclist_t *tcl) {
  if( tcl ) {
    tcl->num = 0;
    return grow_mem((byte_t **)&tcl->list, &tcl->max, sizeof(tempcollect_t), 16);
  }
  return FALSE;
}

bool_t free_tclist(tclist_t *tcl) {
  if( tcl ) {
    if( tcl->list ) {
      free_mem((byte_t **)&tcl->list, &tcl->max);
      tcl->num = 0;
    }
  }
  return FALSE;
}

bool_t reset_tclist(tclist_t *tcl) {
  size_t i;
  if( tcl ) {
    for(i = 0; i < tcl->num; i++) {
      free_tempcollect(&tcl->list[i]);
    }
    tcl->num = 0;
    return TRUE;
  }
  return FALSE;
}

tempcollect_t *get_tclist(tclist_t *tcl, size_t n) {
  return tcl && (n < tcl->num) ? &tcl->list[n] : NULL;
}

tempcollect_t *get_last_tclist(tclist_t *tcl) {
  return tcl && (tcl->num > 0) ? get_tclist(tcl, tcl->num - 1) : NULL;
}

tempcollect_t *find_byname_tclist(tclist_t *tcl, const char_t *name) {
  size_t i;
  if( tcl ) {
    for(i = 0; i < tcl->num; i++) {
      if( strcmp(tcl->list[i].name, name) == 0 ) {
	return &tcl->list[i];
      }
    }
  }
  return NULL;
}

bool_t add_tclist(tclist_t *tcl, const char_t *name, size_t buflen, size_t maxlen) {
  if( tcl ) {
    if( tcl->num >= tcl->max ) {
      grow_mem((byte_t **)&tcl->list, &tcl->max, sizeof(tempcollect_t), 16);
    }
    if( tcl->num < tcl->max ) {
      if( create_tempcollect(&tcl->list[tcl->num], name, buflen, maxlen) ) {
	tcl->num++;
	return TRUE;
      }
    }
  }
  return FALSE;
}

