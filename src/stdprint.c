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
#include "stdprint.h"
#include "stdout.h"
#include "entities.h"

bool_t stdprint_pop_newline(stdparserinfo_t *sp) {
  if( sp && checkflag(sp->reserved,STDPARSE_RESERVED_NEWLINE) ) {
    clearflag(&sp->reserved,STDPARSE_RESERVED_NEWLINE);
    return TRUE;
  }
  return FALSE;
}

bool_t stdprint_pop_verbatim(stdparserinfo_t *sp) {
  if( sp && checkflag(sp->reserved,STDPARSE_RESERVED_VERBATIM) ) {
    clearflag(&sp->reserved,STDPARSE_RESERVED_VERBATIM);
    return TRUE;
  }
  return FALSE;
}

bool_t stdprint_start_tag(stdparserinfo_t *sp, 
			  const char_t *name, const char_t **att) {
  if( sp ) {

    if( stdprint_pop_verbatim(sp) && (sp->depth > 1) ) {
      putc_stdout('\n');
    }

    nputc_stdout('\t', sp->depth - 1);
    write_start_tag_stdout(name, att, FALSE);
    putc_stdout('\n');

    return TRUE;
  }
  return FALSE;
}

bool_t stdprint_end_tag(stdparserinfo_t *sp, const char_t *name) {
  if( sp ) {

    if( stdprint_pop_newline(sp) ) {
      putc_stdout('\n');
    }

    nputc_stdout('\t', sp->depth - 1);
    write_end_tag_stdout(name);
    putc_stdout('\n');

    return TRUE;
  }
  return FALSE;
}

bool_t stdprint_chardata(stdparserinfo_t *sp, const char_t *buf, size_t buflen) {
  if( sp ) {

    if( checkflag(sp->reserved,STDPARSE_RESERVED_VERBATIM) ) {
      write_stdout((byte_t *)buf, buflen);
    } else if( !is_xml_space(buf, buf + buflen) ) {
      nputc_stdout('\t', sp->depth - 1); 
      squeeze_coded_entities_stdout(buf, buflen);
      setflag(&sp->reserved,STDPARSE_RESERVED_NEWLINE);
      return TRUE;
    }

  }
  return FALSE;
}

bool_t stdprint_start_cdata(stdparserinfo_t *sp) {
  if( sp ) {

    nputc_stdout('\t', sp->depth - 1); 

    puts_stdout("<![CDATA[");
    setflag(&sp->reserved,STDPARSE_RESERVED_VERBATIM);

    return TRUE;
  }
  return FALSE;
}

bool_t stdprint_end_cdata(stdparserinfo_t *sp) {
  if( sp ) {
    puts_stdout("]]>");
    clearflag(&sp->reserved,STDPARSE_RESERVED_VERBATIM);

    putc_stdout('\n');

    return TRUE;
  }
  return FALSE;
}

bool_t stdprint_comment(stdparserinfo_t *sp, const char_t *data) {
  if( sp ) {
    puts_stdout("<!--");
    puts_stdout(data);
    puts_stdout("-->");
    setflag(&sp->reserved,STDPARSE_RESERVED_VERBATIM);

    return TRUE;
  }
  return FALSE;
}

bool_t stdprint_dfault(stdparserinfo_t *sp, const char_t *data, size_t buflen) {
  if( sp ) {

    if( sp->depth == 0 ) {
      write_stdout((byte_t *)data, buflen);
      setflag(&sp->reserved,STDPARSE_RESERVED_VERBATIM);
    } else {
      nputc_stdout('\t', sp->depth - 1);
      squeeze_stdout((byte_t *)data, buflen);
    }

    return TRUE;
  }
  return FALSE;
}

bool_t stdprint_pidata(stdparserinfo_t *sp, const char_t *target, const char_t *data) {
  if( sp ) {
    /* tbi */
    return TRUE;
  }
  return FALSE;
}
