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

#ifndef SED_H
#define SED_H

#include "common.h"
#include "cstring.h"

#include <sys/types.h>
#include <regex.h>
#include <wchar.h>

typedef enum { ANY, PTR, INTERVAL } sedadid_t;

#define SEDAD_FLAG_INVERT      0x01

#define SED_FLAG_SUBSTITUTE_ICASE                       0x01
#define SED_FLAG_SUBSTITUTE_PATH                        0x02
#define SED_FLAG_SUBSTITUTE_PATH_STRINGVAL              0x04
#define SED_FLAG_SUBSTITUTE_ALL                         0x08
#define SED_FLAG_TRANSLITERATE_PATH                     0x10
#define SED_FLAG_TRANSLITERATE_PATH_STRINGVAL           0x20


typedef struct sedad_t_ {
  sedadid_t id;
  flag_t flags;
  union {
    struct {
      long line1;
      long line2;
    } range;
    struct sedad_t_ *psedad;
  } args;
} sedad_t; /* a sed address range */

bool_t create_sedad(sedad_t *sc, sedadid_t id);
bool_t free_sedad(sedad_t *sc);


typedef enum { 
  NOP, OPENBLOCK, CLOSEBLOCK, SUBSTITUTE, TRANSLITERATE, 
  QUIT, DELP, PRINTP, APPEND, REPLACE, INSERT, GOTO, LABEL,
  COPYH, COPYHA, PASTEH, PASTEHA, SWAPH, NEXT, LISTP, TEST,
  READF, WRITEF, JOIN, DELJ, PRINTJ
} sedcmdid_t;

typedef struct {
  sedcmdid_t id;
  sedad_t address;
  flag_t flags;
  union {
    struct {
      regex_t find;
      cstring_t replace;
    } s;
    struct {
      wchar_t *from;
      wchar_t *to;
    } y;
    struct {
      cstring_t string;
    } lit;
    struct {
      cstring_t name;
    } file;
  } args;
} sedcmd_t; /* a sed command */

bool_t create_sedcmd(sedcmd_t *sc, sedcmdid_t id, sedadid_t ad);
bool_t free_sedcmd(sedcmd_t *sc);

typedef struct {
  sedcmd_t *list;
  size_t num;
  size_t max;
} sclist_t;

bool_t create_sclist(sclist_t *scl);
bool_t free_sclist(sclist_t *scl);
bool_t reset_sclist(sclist_t *scl);

sedcmd_t *get_sclist(sclist_t *scl, size_t n);
sedcmd_t *get_last_sclist(sclist_t *scl);
bool_t add_sclist(sclist_t *scl);
bool_t pop_sclist(sclist_t *scl);

const char_t *compile_sedad(sedad_t *sa, const char_t *begin, const char_t *end);
const char_t *compile_sedcmd(sedcmd_t *sc, const char_t *begin, const char_t *end);
#endif
