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
#include "rcm.h"
#include "stdprint.h"
#include "stdout.h"
#include "rollback.h"
#include "myerror.h"
#include "io.h"
#include "tempcollect.h"
#include "parser.h"
#include "wrap.h"

#include <string.h>

bool_t create_rcm(rcm_t *rcm) {
  if( rcm ) {
    memset(rcm, 0, sizeof(rcm_t));
    create_stringlist(&rcm->sl);
    create_cstring(&rcm->av, "", 64);
    return TRUE;
  }
  return FALSE;
}

bool_t free_rcm(rcm_t *rcm) {
  if( rcm ) {
    free_stringlist(&rcm->sl);
    free_cstring(&rcm->av);
    return TRUE;
  }
  return FALSE;
}

bool_t reset_rcm(rcm_t *rcm) {
  if( rcm ) {
    rcm->fd = -1;
    clearflag(&rcm->flags,RCM_SELECT);
    clearflag(&rcm->flags,RCM_CP_OKINSERT);
    rcm->depth = 0;
    reset_stringlist(&rcm->sl);
    truncate_cstring(&rcm->av, 0);
  }
  return TRUE;
}

bool_t check_insert_fun(void *user, byte_t *buf, size_t buflen) {
  parser_t *parser = (parser_t *)user;
  return do_parser2(parser, buf, buflen);
}

bool_t insert_rcm(rcm_t *rcm, tempcollect_t *ins) {
  parser_t parser;
  tempcollect_adapter_t ad = { check_insert_fun, NULL};
  if( rcm && ins ) {
    rcm->insert = ins;
    if( is_empty_tempcollect(ins) ) {
      errormsg(E_WARNING, "source data is empty! (check paths?)\n");
    } else if( create_parser(&parser, NULL) ) {
      ad.user = &parser;
      if( write_adapter_tempcollect(ins, &ad) ) {
	setflag(&rcm->flags,RCM_CP_WFXML);
      }
      free_parser(&parser);
    }
    return TRUE;
  }
  return FALSE;
}

/* If include, removes attributes that do not match selection,
 * If !include (exclude), removes attributes that do match selection.
 */
const char_t **filter_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t **att, bool_t include) {
  const char_t *path;
  bool_t ok;
  if( rcm && sp && sp->sel.attrib ) {
    path = string_xpath(&sp->cp);
    reset_stringlist(&rcm->sl);
    do {
      ok = check_xattributelist(&sp->sel.atts, path, att[0]);
      if( ok == include ) {
	add_stringlist(&rcm->sl, att[0], STRINGLIST_DONTFREE);
	add_stringlist(&rcm->sl, att[1], STRINGLIST_DONTFREE);
      }
      att += 2;
    } while( att && *att );
    return argv_stringlist(&rcm->sl);
  }
  return att;
}


bool_t rm_start_tag_rcm(rcm_t *rcm, stdparserinfo_t *sp,
			const char_t *name, const char_t **att) {
  const char_t **fatt;
  if( rcm ) {
    if( (sp->depth == 1) || !sp->sel.active ) {
      if( checkflag(rcm->flags,RCM_RM_OUTPUT) ) {
	fatt = filter_rcm(rcm, sp, att, FALSE);
	write_start_tag_stdout(name, fatt, FALSE);
      }
    }
    return TRUE;
  }
  return FALSE;
}

bool_t rm_end_tag_rcm(rcm_t *rcm, stdparserinfo_t *sp,
		      const char_t *name) {
  if( rcm ) {
    if( (sp->depth == 1) || !sp->sel.active ) {
      if( checkflag(rcm->flags,RCM_RM_OUTPUT) ) {
	write_end_tag_stdout(name);
      }
    }
    return TRUE;
  }
  return FALSE;
}

bool_t rm_attribute_rcm(rcm_t *rcm, stdparserinfo_t *sp,
			const char_t *name, const char_t *value) {
  if( rcm ) {
    return TRUE;
  }
  return FALSE;
}

bool_t rm_chardata_rcm(rcm_t *rcm, stdparserinfo_t *sp,
		       const char_t *buf, size_t buflen) {
  if( rcm ) {
    if( sp->depth == 0 ) {
      if( checkflag(rcm->flags,RCM_RM_OUTPUT) ) {
	write_coded_entities_stdout(buf, buflen);
      }
    } else {
      if( !sp->sel.active ) {
	if( checkflag(rcm->flags,RCM_RM_OUTPUT) ) {
	  write_coded_entities_stdout(buf, buflen);
	}
      }
    }
    return TRUE;
  }
  return FALSE;
}

bool_t rm_dfault_rcm(rcm_t *rcm, stdparserinfo_t *sp,
		     const char_t *data, size_t buflen) {
  if( rcm ) {
    if( sp->depth == 0 ) {
      if( checkflag(rcm->flags,RCM_RM_OUTPUT) ) {
	write_stdout((byte_t *)data, buflen);
      }
    } else {
      if( !sp->sel.active ) {
	if( checkflag(rcm->flags,RCM_RM_OUTPUT) ) {
	  write_stdout((byte_t *)data, buflen);
	}
      }
    }
    return TRUE;
  }
  return FALSE;
}

bool_t rm_start_file_rcm(rcm_t *rcm, const char_t *file) {
  if( rcm ) {
    reset_rcm(rcm);

    if( checkflag(rcm->flags, RCM_WRITE_FILES) ) {

      rcm->fd = (strcmp(file, "stdout") == 0) ? STDOUT_FILENO :
	open_file_with_rollback(file);
      
      if( rcm->fd != -1 ) {
	/* NB: close stdout before closing file */
	open_redirect_stdout(rcm->fd); 
      } else {
	errormsg(E_FATAL, "unable to safely write file %s\n", file);
      }

      setflag(&rcm->flags, RCM_RM_OUTPUT);

    } else {

      open_stdout();

    }

    return TRUE;
  }
  return FALSE;
}

bool_t rm_end_file_rcm(rcm_t *rcm, const char_t *file) {
  if( rcm ) {

    close_stdout();

    if( (rcm->fd != -1) && (rcm->fd != STDOUT_FILENO) ) {
      commit_file_with_rollback(rcm->fd);
      close_file_with_rollback(rcm->fd);
      rcm->fd = -1;
    }

    return TRUE;
  }
  return FALSE;
}

typedef struct {
  cstring_t *cs;
  const char_t *p;
  tempcollect_t *tc;
} dump_cstring_fun_t;

bool_t dump_cstring_fun(void *user, byte_t *buf, size_t buflen) {
  dump_cstring_fun_t *wcef = (dump_cstring_fun_t *)user;
  if( wcef ) {
    wcef->p = 
      write_coded_entities_cstring(wcef->cs, wcef->p, (char_t *)buf, buflen);
  }
  return (wcef->p != NULL);
}

bool_t dump_cstring_tempcollect(cstring_t *cs, tempcollect_t *tc) {
  dump_cstring_fun_t wcef;
  tempcollect_adapter_t ad;
  if( cs && tc ) {
    wcef.cs = cs;
    wcef.p = begin_cstring(cs);
    wcef.tc = tc;
    ad.fun = dump_cstring_fun;
    ad.user = &wcef;
    return write_adapter_tempcollect(tc, &ad);
  }
  return FALSE;
}

const char_t **try_write_attribute(rcm_t *rcm, stdparserinfo_t *sp,
				   const char_t **att) {
  const char_t *path, *q;
  xattribute_t *xa;
  int i;
  if( rcm && rcm->insert && !checkflag(rcm->flags,RCM_CP_OKINSERT) ) {
    path = string_xpath(&sp->cp);
    reset_stringlist(&rcm->sl);
    for(i = 0; i < sp->sel.atts.num; i++) {
      xa = &sp->sel.atts.list[i];
      if( xa->begin &&
	  (0 == match_no_att_no_pred_xpath(xa->path, xa->begin, path)) ) {
	if( false_and_setflag(&rcm->flags,RCM_CP_OKINSERT) ) {
	  if( dump_cstring_tempcollect(&rcm->av, rcm->insert) ) {
	    q = strndup(xa->begin + 1, xa->end - xa->begin - 1);
	    add_stringlist(&rcm->sl, q, STRINGLIST_FREE);
	    q = strndup(p_cstring(&rcm->av), buflen_cstring(&rcm->av));
	    add_stringlist(&rcm->sl, q, STRINGLIST_FREE);
	  } else {
	    errormsg(E_WARNING, 
		     "source data could not be inserted in attribute %.*s\n",
		     xa->end - xa->begin, xa->begin);
	    /* old attribute value will be used */
	  }
	  if( checkflag(rcm->flags,RCM_CP_MULTI) ) {
	    clearflag(&rcm->flags,RCM_CP_OKINSERT);
	  }
	}
      }
    }
    while(att && *att) {
      if( -1 == find_stringlist(&rcm->sl, att[0]) ) {
	add_stringlist(&rcm->sl, strdup(att[0]),STRINGLIST_FREE);
	add_stringlist(&rcm->sl, strdup(att[1]),STRINGLIST_FREE);
      }
      att += 2;
    }

    return argv_stringlist(&rcm->sl);
  }
  return att;
}

void try_write(rcm_t *rcm) {
  if( rcm && rcm->insert ) {
    if( false_and_setflag(&rcm->flags,RCM_CP_OKINSERT) ) {
      write_stdout_tempcollect(rcm->insert);
      if( checkflag(rcm->flags,RCM_CP_MULTI) ) {
	clearflag(&rcm->flags,RCM_CP_OKINSERT);
      }
    }
  }  
}

/* selection starts when depth=1 and SELECT flag not set */
bool_t start_of_selection(rcm_t *rcm) {
  return( (rcm->depth == 1) && false_and_setflag(&rcm->flags,RCM_SELECT) );
}

/* selection ends when depth=1 or depth=0 and SELECT flag set */
bool_t end_of_selection(rcm_t *rcm) {
  return( (rcm->depth <= 1) && true_and_clearflag(&rcm->flags,RCM_SELECT) );
}

bool_t cp_start_tag_rcm(rcm_t *rcm, stdparserinfo_t *sp,
			const char_t *name, const char_t **att) {
  const char_t **fatt;
  if( rcm ) {
    rcm->depth += (sp->sel.active ? 1 : 0);
    if( checkflag(rcm->flags,RCM_CP_OUTPUT) ) {
      fatt = try_write_attribute(rcm, sp, att);
      if( !sp->sel.active ||
      	  !checkflag(rcm->flags,RCM_CP_REPLACE) ||
	  ((sp->depth == 1) && !checkflag(rcm->flags,RCM_CP_WFXML)) ) {
      	write_start_tag_stdout(name, fatt, FALSE);
      }

      if( start_of_selection(rcm) ) {
	if( checkflag(rcm->flags,RCM_CP_PREPEND) ) {
	  try_write(rcm);
	}
      }

    }
    return TRUE;
  }
  return FALSE;
}

bool_t cp_end_tag_rcm(rcm_t *rcm, stdparserinfo_t *sp,
		      const char_t *name) {
  bool_t eos;
  if( rcm ) {
    if( checkflag(rcm->flags,RCM_CP_OUTPUT) ) {

      eos = end_of_selection(rcm);
      if( eos ) {
	if( checkflag(rcm->flags,RCM_CP_APPEND) ) {
	  try_write(rcm);
	}
      }

      if( (sp->depth == 1) && sp->sel.active) {
	try_write(rcm);
      }

      if( !sp->sel.active || 
	  !checkflag(rcm->flags,RCM_CP_REPLACE) ||
	  ((sp->depth == 1) && !checkflag(rcm->flags,RCM_CP_WFXML)) ) {
	write_end_tag_stdout(name);
      }

      if( eos && !checkflag(rcm->flags,RCM_CP_MULTI) ) {
      	clearflag(&rcm->flags,RCM_CP_REPLACE);
      }
    }
    rcm->depth -= (sp->sel.active ? 1 : 0);
    return TRUE;
  }
  return FALSE;
}

bool_t cp_attribute_rcm(rcm_t *rcm, stdparserinfo_t *sp,
			const char_t *name, const char_t *value) {
  if( rcm ) {
    return TRUE;
  }
  return FALSE;
}

bool_t cp_chardata_rcm(rcm_t *rcm, stdparserinfo_t *sp,
		     const char_t *buf, size_t buflen) {
  if( rcm ) {
    rcm->depth += (sp->sel.active ? 1 : 0);

    if( checkflag(rcm->flags,RCM_CP_OUTPUT) ) {

      if( start_of_selection(rcm) ) {
	/* if selection starts in a chardata, pretend it started
	   in the surrounding tag */
	rcm->depth++;

	if( checkflag(rcm->flags,RCM_CP_PREPEND) ) {
	  try_write(rcm);
	}
      }

      if( !sp->sel.active || !checkflag(rcm->flags,RCM_CP_REPLACE) ) {
	write_coded_entities_stdout(buf, buflen);
      }

      /* end of selection: see endtag */

    }

    rcm->depth -= (sp->sel.active ? 1 : 0);
    return TRUE;
  }
  return FALSE;
}

bool_t cp_dfault_rcm(rcm_t *rcm, stdparserinfo_t *sp,
		     const char_t *data, size_t buflen) {
  if( rcm ) {
    if( checkflag(rcm->flags,RCM_CP_OUTPUT) ) {

      if( !sp->sel.active || !checkflag(rcm->flags,RCM_CP_REPLACE) ) {
	write_stdout((byte_t *)data, buflen);
      }

    }
    return TRUE;
  }
  return FALSE;
}

bool_t cp_start_target_rcm(rcm_t *rcm, const char_t *file) {
  if( rcm ) {
    reset_rcm(rcm);

    if( checkflag(rcm->flags, RCM_WRITE_FILES) ) {

      rcm->fd = (strcmp(file, "stdout") == 0) ? STDOUT_FILENO :
	open_file_with_rollback(file);
      
      if( rcm->fd != -1 ) {
	/* NB: close stdout before closing file */
	open_redirect_stdout(rcm->fd); 
      } else {
	errormsg(E_FATAL, "unable to safely write file %s\n", file);
      }

      setflag(&rcm->flags, RCM_CP_OUTPUT);

    } else {

      open_stdout();

    }

    return TRUE;
  }
  return FALSE;
}

bool_t cp_end_target_rcm(rcm_t *rcm, const char_t *file) {
  if( rcm ) {

    close_stdout();

    if( rcm->fd != -1 ) {
      commit_file_with_rollback(rcm->fd);
      close_file_with_rollback(rcm->fd);
      rcm->fd = -1;
    }

    return TRUE;
  }
  return FALSE;
}
