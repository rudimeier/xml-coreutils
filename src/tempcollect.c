/* 
 * Copyright (C) 2006 Laird Breyer
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
    tc->tempfile = NULL;
    return (tc->buf != NULL);
  }
  return FALSE;
}

bool_t free_tempcollect(tempcollect_t *tc) {
  if( tc ) {
    close_file_tempcollect(tc);
    if( tc->buf ) {
      free_mem(&tc->buf, &tc->buflen);
    }
    return TRUE;
  }
  return FALSE;
}

bool_t reset_tempcollect(tempcollect_t *tc) {
  if( tc ) {
    close_file_tempcollect(tc);
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
      flush_file_tempcollect(tc);
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

bool_t open_file_tempcollect(tempcollect_t *tc) {
  if( tc ) {
    if( tc->tempfile == NULL ) {
      tc->tempfile = tmpfile();
    }
    return (tc->tempfile != NULL);
  }
  return FALSE;
}

bool_t flush_file_tempcollect(tempcollect_t *tc) {
  size_t n, w;
  if( tc && (tc->bufpos > 0) ) {
    if( open_file_tempcollect(tc) ) {
      if( -1 == fseek(tc->tempfile, 0, SEEK_END) ) {
	errormsg(E_ERROR,
		 "couldn't seek in temporary file, some data is lost.\n");
	return FALSE;
      }
      for(n = 0; n < tc->bufpos; ) {
	w = fwrite(tc->buf + n, sizeof(byte_t), tc->bufpos - n, tc->tempfile);
	if( w == -1 ) {
	  errormsg(E_ERROR, 
		   "couldn't write temporary file, some data is lost.\n");
	  return FALSE;
	}
	n += w;
      }
      tc->bufpos = 0;
    }
  }
  return FALSE;
}

bool_t rewind_file_tempcollect(tempcollect_t *tc) {
  if( tc && tc->tempfile ) {
    if( -1 == fseek(tc->tempfile, 0, SEEK_SET) ) {
      errormsg(E_ERROR,
	       "couldn't seek in temporary file.\n");
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t load_file_tempcollect(tempcollect_t *tc) {
  size_t n, r;
  if( tc && tc->tempfile && tc->buf ) {
    for(n = 0; n + tc->bufpos < tc->buflen; ) {
      r = fread(tc->buf + (tc->bufpos + n), sizeof(byte_t), 
		tc->buflen - (tc->bufpos + n), tc->tempfile);
      if( r <= 0 ) {
	break;
      }
      n += r;
    }
    if( r < 0 ) {
      errormsg(E_ERROR,
	       "couldn't read temporary file.\n");
      return FALSE;
    }
    tc->bufpos += n;
    return (n > 0);
  }
  return FALSE;
}

bool_t close_file_tempcollect(tempcollect_t *tc) {
  if( tc ) {
    if( tc->tempfile ) {
      fclose(tc->tempfile);
      tc->tempfile = NULL;
      return TRUE;
    }
  }
  return FALSE;
}

bool_t write_stdout_tempcollect(tempcollect_t *tc) {
  bool_t aok = TRUE;
  if( tc && tc->buf ) {
    if( tc->tempfile ) {
      flush_file_tempcollect(tc);
      if( rewind_file_tempcollect(tc) ) {
	while( aok && load_file_tempcollect(tc) ) {
	  aok = write_stdout(tc->buf, tc->bufpos);
	  tc->bufpos = 0;
	}
      }
      return aok;
    }
    return write_stdout(tc->buf, tc->bufpos);
  }
  return FALSE;
}

bool_t squeeze_stdout_tempcollect(tempcollect_t *tc) {
  bool_t aok = TRUE;
  if( tc && tc->buf ) {
    if( tc->tempfile ) {
      flush_file_tempcollect(tc);
      if( rewind_file_tempcollect(tc) ) {
	while( aok && load_file_tempcollect(tc) ) {
	  aok = squeeze_stdout(tc->buf, tc->bufpos);
	  tc->bufpos = 0;
	}
      }
      return aok;
    }
    return squeeze_stdout(tc->buf, tc->bufpos);
  }
  return FALSE;
}

bool_t create_tclist(tclist_t *tcl) {
  if( tcl ) {
    tcl->num = 0;
    return grow_mem(&tcl->list, &tcl->max, sizeof(tempcollect_t), 16);
  }
  return FALSE;
}

bool_t free_tclist(tclist_t *tcl) {
  if( tcl ) {
    if( tcl->list ) {
      free_mem(&tcl->list, &tcl->max);
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
      grow_mem(&tcl->list, &tcl->max, sizeof(tempcollect_t), 16);
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

