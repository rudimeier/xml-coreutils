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

#ifndef RCM_H
#define RCM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "stdparse.h"
#include "tempcollect.h"
#include "stringlist.h"
#include <stdio.h>

typedef struct {
  int fd;
  tempcollect_t *insert;
  flag_t flags;
  int depth;
  stringlist_t sl;
  cstring_t av;
} rcm_t;

#define RCM_SELECT       0x001 /* true if we are within a selection */
#define RCM_CP_OKINSERT  0x002 /* true if we copied already */

#define RCM_WRITE_FILES  0x004 /* set this if you want output to a file */
#define RCM_RM_OUTPUT    0x008 /* if unset, rm_*() don't print anything */
#define RCM_CP_OUTPUT    0x010 /* if unset, cp_*() don't print anything */

#define RCM_CP_PREPEND   0x020 /* if set, want to prepend existing data */
#define RCM_CP_REPLACE   0x040 /* if set, want to replace existing data */
#define RCM_CP_APPEND    0x080 /* if set, want to append existing data */

#define RCM_CP_MULTI     0x100 /* if set, want to copy data multiple times */

#define RCM_CP_WFXML     0x200 /* if set, rcm->insert is well formed xml */

bool_t create_rcm(rcm_t *rcm);
bool_t free_rcm(rcm_t *rcm);

bool_t insert_rcm(rcm_t *rcm, tempcollect_t *ins);

bool_t rm_start_tag_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *name, const char_t **att);
bool_t rm_end_tag_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *name);
bool_t rm_attribute_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *name, const char_t *value);
bool_t rm_chardata_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *buf, size_t buflen);
bool_t rm_dfault_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *data, size_t buflen);
bool_t rm_start_file_rcm(rcm_t *rcm, const char_t *file);
bool_t rm_end_file_rcm(rcm_t *rcm, const char_t *file);

bool_t cp_start_tag_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *name, const char_t **att);
bool_t cp_end_tag_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *name);
bool_t cp_attribute_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *name, const char_t *value);
bool_t cp_chardata_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *buf, size_t buflen);
bool_t cp_dfault_rcm(rcm_t *rcm, stdparserinfo_t *sp, const char_t *data, size_t buflen);
bool_t cp_start_target_rcm(rcm_t *rcm, const char_t *file);
bool_t cp_end_target_rcm(rcm_t *rcm, const char_t *file);

#endif
