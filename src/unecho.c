/* 
 * Copyright (C) 2009 Laird Breyer
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
#include "unecho.h"
#include "cstring.h"

bool_t create_unecho(unecho_t *ue, flag_t flags) {
  bool_t ok = TRUE;
  if( ue ) {
    ok &= create_xpath(&ue->cp);
    ok &= create_tempvar(&ue->sv, "sv", MINVARSIZE, MAXVARSIZE);
    ue->flags = flags;
    return ok; 
  }
  return FALSE;
}

bool_t reset_unecho(unecho_t *ue, flag_t flags) {
  if( ue ) {
    reset_xpath(&ue->cp);
    reset_tempvar(&ue->sv);
    ue->flags = flags;
    return TRUE; 
  }
  return FALSE;
}

bool_t free_unecho(unecho_t *ue) {
  if( ue ) {
    free_tempvar(&ue->sv);
    free_xpath(&ue->cp);
    memset(ue, 0, sizeof(unecho_t));
    return TRUE;
  }
  return FALSE;
}

bool_t format_path_unecho(unecho_t *ue, const xpath_t *xpath) {
  if( ue ) {
    if( !equal_xpath(&ue->cp, xpath) ) {
      /* puts_stdout("not-identical\n"); */

      reset_tempvar(&ue->sv);

      if( checkflag(ue->flags,UNECHO_FLAG_ABSOLUTE) ) {
      	copy_xpath(&ue->cp, xpath);
      } else {
      	retarget_xpath(&ue->cp, xpath);
      }

      putc_tempvar(&ue->sv, '[');
      puts_tempvar(&ue->sv, string_xpath(&ue->cp));
      putc_tempvar(&ue->sv, ']');

      copy_xpath(&ue->cp, xpath);
      return TRUE;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t reconstruct_stringval_unecho(tempvar_t *sv, const char_t *buf, size_t buflen) {
  bool_t ok = TRUE;
  const char_t *p, *q, *r, *code;
  if( sv && buf && (buflen > 0) ) {
    q = buf;
    r = buf + buflen;
    p = q;
    while( *p && (p < r) ) {
      code = 
	(*p == '\n') ? "\\n" :
	(*p == '\t') ? "\\t" :
	NULL;
      p++;
      if( code ) {
	if( p > q ) {
	  /* note write_tempvar returns FALSE if zero length */
	  ok &= ((p - q == 1) || write_tempvar(sv, (byte_t *)q, p - q - 1));
	}
	ok &= write_tempvar(sv, (byte_t *)code, strlen(code));
	q = p;
      }
    }
    if( r > q ) {
      ok &= write_tempvar(sv, (byte_t *)q, r - q);
    }
    return ok;
  }
  return FALSE;
}

bool_t write_unecho(unecho_t *ue, 
		    const xpath_t *xpath, 
		    const byte_t *buf, size_t buflen) {
  if( ue ) {
    if( format_path_unecho(ue, xpath) ) {
      return (buflen > 0) ? write_tempvar(&ue->sv, buf, buflen) : TRUE;
    }
  }
  return FALSE;
}

bool_t reconstruct_unecho(unecho_t *ue, 
			  const xpath_t *xpath, 
			  const byte_t *buf, size_t buflen) {
  if( ue ) {
    if( format_path_unecho(ue, xpath) ) {
      return (buflen > 0) ?
      	reconstruct_stringval_unecho(&ue->sv, (char_t *)buf, buflen) : TRUE;
    }
  }
  return FALSE;
}

bool_t squeeze_unecho(unecho_t *ue, 
		      const xpath_t *xpath, 
		      const byte_t *buf, size_t buflen) {
  if( ue ) {
    /* puts_stdout("squeezing {"); */
    /* puts_stdout(xpath->path); */
    /* puts_stdout("}("); */
    /* puts_stdout(ue->cp.path); */
    /* puts_stdout(")\n"); */

    if( format_path_unecho(ue, xpath) ) {
      squeeze_tempvar(&ue->sv, (char_t *)buf, buflen);
    }
    return TRUE;
  }
  return FALSE;
}
