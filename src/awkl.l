%{
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * Author:   Laird Breyer <laird@lbreyer.com>
 */

#include "awkvm.h"

int current_lineno = 1;
YY_BUFFER_STATE handle;

%}

aNUMBER               [+-]?[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?
aNAME                 [A-Za-z_][A-Za-z0-9_]*
aSTRING               \"[^"\\\n]*(\\.[^"\\\n]*)*\"
aERE                  \/[^/\\\n]*(\\.[^/\\\n]*)*\/
aXPATH                \:[A-Za-z0-9/\*]*

%%

"BEGIN" { return Begin; }
"END" { return End; }
"break" { return Break; }
"continue" { return Continue; }
"delete" { return Delete; }
"do" { return Do; }
"else" { return Else; }
"next" { return Next; }
"print" { return Print; }
"printf" { return Printf; }
"return" { return Return; }
"while" { return While; }
"atan2" { return BUILTIN_FUNC_NAME; }
"cos" { return BUILTIN_FUNC_NAME; }
"sin" { return BUILTIN_FUNC_NAME; }
"exp" { return BUILTIN_FUNC_NAME; }
"log" { return BUILTIN_FUNC_NAME; }
"sqrt" { return BUILTIN_FUNC_NAME; }
"int" { return BUILTIN_FUNC_NAME; }
"rand" { return BUILTIN_FUNC_NAME; }
"srand" { return BUILTIN_FUNC_NAME; }
"gsub" { return BUILTIN_FUNC_NAME; }
"index" { return BUILTIN_FUNC_NAME; }
"length" { return BUILTIN_FUNC_NAME; }
"match" { return BUILTIN_FUNC_NAME; }
"split" { return BUILTIN_FUNC_NAME; }
"sprintf" { return BUILTIN_FUNC_NAME; }
"sub" { return BUILTIN_FUNC_NAME; }
"substr" { return BUILTIN_FUNC_NAME; }
"tolower" { return BUILTIN_FUNC_NAME; }
"toupper" { return BUILTIN_FUNC_NAME; }
"close" { return BUILTIN_FUNC_NAME; }
"system" { return BUILTIN_FUNC_NAME; }
"+=" { return ADD_ASSIGN; }
"-=" { return SUB_ASSIGN; }
"*=" { return MUL_ASSIGN; }
"/=" { return DIV_ASSIGN; }
"%=" { return MOD_ASSIGN; }
"^=" { return POW_ASSIGN; }
"||" { return OR; }
"&&" { return AND; }
"!~" { return NO_MATCH; }
"==" { return EQ; }
"<=" { return LE; }
">=" { return GE; }
"!=" { return NE; }
"++" { return INCR; }
"--" { return DECR; }
">>" { return APPEND; }


[\n]               { current_lineno++; }

{aNUMBER} { return NUMBER; }
{aNAME} { return NAME; }
{aSTRING} { return STRING; }
{aXPATH} { return XPATH; }
{aERE} { return ERE; }

%%

void reset_lexer_awk() {
  current_lineno = 1;
}

void scan_lexer_awk(char *buf, int len) {
  /* note: this strdups the buf */
  handle = yy_scan_bytes(buf, len);
}

void free_lexer_awk() {
  yy_delete_buffer(handle);
}
