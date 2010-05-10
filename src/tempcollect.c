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
#include "myerror.h"
#include "stdout.h"
#include "entities.h"
#include "io.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <stdarg.h>

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

size_t tell_tempcollect(tempcollect_t *tc) {
  return tc->bufpos + (tc->tempfile ? ftell(tc->tempfile) : 0);
}

bool_t truncate_tempcollect(tempcollect_t *tc, size_t newlen) {
  size_t killbytes;
  if( tc ) {
    killbytes = tell_tempcollect(tc) - newlen;
    if( killbytes <= tc->bufpos ) {
      tc->bufpos -= killbytes;
      return TRUE;
    } else if( tc->tempfile ) {
      fseek(tc->tempfile, newlen, SEEK_SET);
      ftruncate(fileno(tc->tempfile), newlen);
      tc->bufpos = 0;
      return TRUE;
    } 
  }
  return FALSE;
}

bool_t reserve_tempcollect(tempcollect_t *tc, int buflen) {
  if( tc && (buflen > 0) ) {
    while( (tc->buflen < tc->max_buflen) &&
	   (tc->bufpos + buflen > tc->buflen) ) {
      if( !grow_mem(&tc->buf, &tc->buflen, sizeof(byte_t), 128) ) {
	break;
      }
    }
    if( tc->bufpos + buflen > tc->buflen ) {
      flush_file_tempcollect(tc);
    }
    return (tc->bufpos + buflen <= tc->buflen);
  }
  return (tc && (buflen <= 0));
}

/* returns FALSE if buflen is zero, ie TRUE if buffer was actually written */
bool_t write_tempcollect(tempcollect_t *tc, const byte_t *buf, size_t buflen) {
  if( tc && buf && (buflen > 0) ) {
    if( !reserve_tempcollect(tc, buflen) ) {
      errormsg(E_WARNING, 
	       "tempcollect buffer overflow, some data may be discarded.\n"); 
      buflen = tc->buflen - tc->bufpos;
    }
    memcpy(tc->buf + tc->bufpos, buf, buflen);
    tc->bufpos += buflen;
    return (bool_t)(buflen > 0);
  }
  return FALSE;
}

bool_t puts_tempcollect(tempcollect_t *tc, const char_t *s) {
  return write_tempcollect(tc, (byte_t *)s, s ? strlen(s) : 0);
}

bool_t putc_tempcollect(tempcollect_t *tc, char_t c) {
  return write_tempcollect(tc, (byte_t *)&c, 1);
}

bool_t nputc_tempcollect(tempcollect_t *tc, char_t c, size_t n) {
  bool_t ok = TRUE;
  size_t i;
  for(i = 0; i < n; i++) {
    ok = ok && write_tempcollect(tc, (byte_t *)&c, 1);
  }
  return ok;
}

bool_t nprintf_tempcollect(tempcollect_t *tc, int size, const char_t *fmt, ...) {
  va_list vap;
  int n = size;


#if HAVE_VPRINTF

  size = tc->buflen - tc->bufpos;

  if( (n < size) || reserve_tempcollect(tc, n) ) {
    size = tc->buflen - tc->bufpos; /* after reserve */
    va_start(vap, fmt);
    n = vsnprintf((char *)(tc->buf + tc->bufpos), size, fmt, vap);
    va_end(vap);
    if( (n > size) && reserve_tempcollect(tc, n) ) {
      size = tc->buflen - tc->bufpos;
      va_start(vap, fmt);
      n = vsnprintf((char *)(tc->buf + tc->bufpos), size, fmt, vap);
      va_end(vap);
    }
  }

  if( n <= size ) {
    tc->bufpos += n;
    return TRUE;
  }

#endif

  return FALSE;
}

bool_t squeeze_tempcollect(tempcollect_t *tc, 
			   const char_t *buf, size_t buflen) {
  const char_t *p, *q;
  bool_t w; /* true if start or last written char is whitespace */

  if( tc && buf && (buflen > 0) ) {

    while( (tc->buflen < tc->max_buflen) &&
	   (tc->bufpos + buflen > tc->buflen) ) {
      if( !grow_mem(&tc->buf, &tc->buflen, sizeof(byte_t), 128) ) {
	break;
      }
    }

    w = ( (tc->bufpos == 0) ? 
	  (tc->tempfile == NULL) : xml_whitespace(tc->buf[tc->bufpos - 1]) );

    if( tc->bufpos + buflen > tc->buflen ) {
      flush_file_tempcollect(tc);
    }

    if( tc->bufpos + buflen > tc->buflen ) {
      errormsg(E_WARNING, 
	       "tempcollect buffer overflow, some data may be discarded.\n"); 
      buflen = tc->buflen - tc->bufpos;
    }

    p = (char_t *)buf;
    q = (char_t *)buf + buflen;
    if( w ) {
      p = skip_xml_whitespace(p,q);
    }
    while( p < q ) { /* bufpos cannot go wild */
      tc->buf[tc->bufpos++] = 
	xml_whitespace(*p) ? ((p = skip_xml_whitespace(++p,q)),' ') : *p++;
    }

    return (bool_t)(buflen > 0);
  }
  return FALSE;
}

bool_t write_start_tag_tempcollect(tempcollect_t *tc, const char_t *name, const char_t **att) {
  bool_t ok = TRUE;
  if( tc && name ) {

    ok &= putc_tempcollect(tc, '<');
    ok &= puts_tempcollect(tc, name);

    while( att && *att ) {
      ok &= putc_tempcollect(tc, ' ');
      ok &= puts_tempcollect(tc, att[0]);
      ok &= puts_tempcollect(tc, "=\"");
      ok &= puts_tempcollect(tc, att[1]);
      ok &= putc_tempcollect(tc, '\"');
      att += 2;
    }

    ok &= puts_tempcollect(tc, ">");

    return ok;
  }
  return FALSE;
}

bool_t write_end_tag_tempcollect(tempcollect_t *tc, const char_t *name) {
  bool_t ok = TRUE;
  if( tc && name ) {

    ok &= puts_tempcollect(tc, "</");
    ok &= puts_tempcollect(tc, name);
    ok &= puts_tempcollect(tc, ">");

    return ok;
  }
  return FALSE;
}

bool_t write_coded_entities_tempcollect(tempcollect_t *tc, const char_t *buf, size_t buflen) {
  const char_t *p;
  const char_t *end = buf + buflen;

  if( buf && end && (buf < end) ) {
    do {
      p = find_next_special(buf, end);
      if( p && (p < end) ) {
	write_tempcollect(tc, (byte_t *)buf, p - buf);
	puts_tempcollect(tc, get_entity(*p));
	buf = p;
	buf++;
      } else {
	write_tempcollect(tc, (byte_t *)buf, end - buf);
	buf = end;
      }
    } while( buf < end ); 
    return TRUE;
  }
  return FALSE;
}

bool_t is_empty_tempcollect(tempcollect_t *tc) {
  return tc ? ((tc->bufpos == 0) && (tc->tempfile == NULL)) : TRUE;
}

bool_t open_file_tempcollect(tempcollect_t *tc) {
  int fl;
  if( tc ) {
    if( tc->tempfile == NULL ) {
      tc->tempfile = tmpfile();
      if( tc->tempfile ) {
	/* this tempfile should be private */
	fl = fcntl(fileno(tc->tempfile), F_GETFD);
	fcntl(fileno(tc->tempfile), F_SETFD, fl|FD_CLOEXEC);
      }
    }
    return (tc->tempfile != NULL);
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


/* return const pointers to a portion of the memory, but only if
   no backing file is present. 
*/ 
bool_t peek_tempcollect(tempcollect_t *tc, size_t start, size_t end,
			const byte_t **bufstart, const byte_t **bufend) {
  if( tc && !tc->tempfile && 
      bufstart && bufend && 
      (0 <= start) && (start <= end) ) {
    *bufstart = tc->buf + start;
    *bufend = tc->buf + tc->bufpos;
    return (start <= tc->bufpos); 
  }
  return FALSE;
}


bool_t write_stdout_fun(void *user, byte_t *buf, size_t buflen) {
  return write_stdout(buf, buflen);
}

bool_t write_stdout_tempcollect(tempcollect_t *tc) {
  tempcollect_adapter_t ad = { write_stdout_fun, NULL };
  return write_adapter_tempcollect(tc, &ad);
}

bool_t squeeze_stdout_fun(void *user, byte_t *buf, size_t buflen) {
  return squeeze_stdout(buf, buflen);
}

bool_t squeeze_stdout_tempcollect(tempcollect_t *tc) {
  tempcollect_adapter_t ad = { squeeze_stdout_fun, NULL };
  return write_adapter_tempcollect(tc, &ad);
}

/* appends tc with the contents of file. If a read problem occurs,
 * the data is truncated back to the pre-read state.
 */
bool_t read_from_file_tempcollect(tempcollect_t *tc, const char_t *file) {
  stream_t strm;
  byte_t *buf;
  size_t ipos, bufsize;
  bool_t done = FALSE;
  if( tc && file ) {
    if( open_file_stream(&strm, file) ) {
      ipos = tell_tempcollect(tc);
      bufsize = strm.blksize;
      while( reserve_tempcollect(tc, bufsize) ) {
	buf = tc->buf + tc->bufpos;
	if( read_stream(&strm, buf, bufsize) ) {
	  tc->bufpos += strm.buflen;
	} else {
	  done = TRUE;
	  break;
	}
      }
      close_stream(&strm);
      if( !done ) { truncate_tempcollect(tc, ipos); }
      return done;
    }
  }
  return FALSE;
}

typedef struct {
  int fd;
  tempcollect_t *tc;
} write_file_fun_t;

bool_t write_file_fun(void *user, byte_t *buf, size_t buflen) {
  write_file_fun_t *wff = (write_file_fun_t *)user;
  return wff ? write_file(wff->fd, buf, buflen) : FALSE;
}

/* append a file with the contents of tc. */
bool_t write_to_file_tempcollect(tempcollect_t *tc, 
				 const char_t *file, int openflags) {
  write_file_fun_t wff;
  tempcollect_adapter_t ad;
  bool_t retval;
  if( tc && file ) {
    wff.fd = open(file, openflags);
    if( wff.fd != -1 ) {
      wff.tc = tc;
      ad.fun = write_file_fun;
      ad.user = &wff;
      retval = write_adapter_tempcollect(tc, &ad);
      close(wff.fd);
      return retval;
    }
  }
  return FALSE;
}

bool_t write_adapter_tempcollect(tempcollect_t *tc, 
				 tempcollect_adapter_t *ad) {
  bool_t aok = TRUE;
  if( tc && tc->buf && ad && ad->fun ) {
    if( tc->tempfile ) {
      flush_file_tempcollect(tc);
      if( rewind_file_tempcollect(tc) ) {
	while( aok && load_file_tempcollect(tc) ) {
	  aok = ad->fun(ad->user, tc->buf, tc->bufpos);
	  tc->bufpos = 0;
	}
      }
      return aok;
    }
    return ad->fun(ad->user, tc->buf, tc->bufpos);
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

