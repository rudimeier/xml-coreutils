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
#include <stdio.h>

/* The awkp.y grammar here is adapted from the official awk grammar published
 * in IEEE Std 1003.1 (2004). HOWEVER, xml-awk IS NOT A CONFORMING 
 * IMPLEMENTATION, AND SIGNIFICANT DEPARTURES FROM THE STANDARD ARE
 * EXPECTED EVENTUALLY (WHERE THIS MAKES SENSE FOR XML INPUTS).
 */

static awkvm_t *avm = NULL; /* local copy of the VM which receives the code */

#define YYDEBUG 0

extern char *inputfile;
extern long inputline;

extern FILE *yyin;
extern char *yytext;
extern int yylex(void);

/* defined in awk-lexer.l */

  /* defined here */
  int yyerror(char *s);

  %}


%union {
  int  num;
  char *str;
}

%token NAME NUMBER STRING ERE XPATH
%token FUNC_NAME   /* Name followed by '(' without white space. */


/* Keywords  */
%token       Begin   End
/*          'BEGIN' 'END'                            */

%token       Break   Continue   Delete   Do   Else
/*          'break' 'continue' 'delete' 'do' 'else'  */

%token       Exit   For   Function   If   In
/*          'exit' 'for' 'function' 'if' 'in'        */

%token       Next   Print   Printf   Return   While
/*          'next' 'print' 'printf' 'return' 'while' */

/* Reserved function names */
%token BUILTIN_FUNC_NAME
              /* One token for the following:
               * atan2 cos sin exp log sqrt int rand srand
               * gsub index length match split sprintf sub
               * substr tolower toupper close system
               */
%token GETLINE
              /* Syntactically different from other built-ins. */

/* Two-character tokens. */
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN POW_ASSIGN
/*     '+='       '-='       '*='       '/='       '%='       '^=' */

%token OR   AND  NO_MATCH   EQ   LE   GE   NE   INCR  DECR  APPEND
/*     '||' '&&' '! ' '==' '<=' '>=' '!=' '++'  '--'  '>>'   */

/* One-character tokens. */
%token '{' '}' '(' ')' '[' ']' ',' ';' NEWLINE
%token '+' '-' '*' '%' '^' '!' '>' '<' '|' '?' ':' ' ' '$' '='

%start program

%right ASSIGN 
%right '?' ':'
%left OR
%left AND
%left GETLINE
%left STRING NUMBER
%left '+' '-'
%left '*' '/' '%'
%right '^'
%left INCR DECR
%left '$'
%left '(' ')'

%%


  program          : item_list
                   | actionless_item_list
                   ;


  item_list        : newline_opt
                   | actionless_item_list item terminator
                   | item_list            item terminator
                   | item_list          action terminator
                   ;


  actionless_item_list : item_list            pattern terminator
                   | actionless_item_list pattern terminator
                   ;


  item             : pattern action
                   | Function NAME      '(' param_list_opt ')'
                         newline_opt action
                   | Function FUNC_NAME '(' param_list_opt ')'
                         newline_opt action
                   ;


  param_list_opt   : /* empty */
                   | param_list
                   ;


  param_list       : NAME
                   | param_list ',' NAME
                   ;


  pattern          : Begin
                   | End
                   | expr
                   | expr ',' newline_opt expr
                   ;


  action           : '{' newline_opt                             '}'
                   | '{' newline_opt terminated_statement_list   '}'
                   | '{' newline_opt unterminated_statement_list '}'
                   ;


  terminator       : terminator ';'
                   | terminator NEWLINE
                   |            ';'
                   |            NEWLINE
                   ;


  terminated_statement_list : terminated_statement
                   | terminated_statement_list terminated_statement
                   ;


  unterminated_statement_list : unterminated_statement
                   | terminated_statement_list unterminated_statement
                   ;


  terminated_statement : action newline_opt
                   | If '(' expr ')' newline_opt terminated_statement
                   | If '(' expr ')' newline_opt terminated_statement
                         Else newline_opt terminated_statement
                   | While '(' expr ')' newline_opt terminated_statement
                   | For '(' simple_statement_opt ';'
                        expr_opt ';' simple_statement_opt ')' newline_opt
                        terminated_statement
                   | For '(' NAME In NAME ')' newline_opt
                        terminated_statement
                   | ';' newline_opt
                   | terminatable_statement NEWLINE newline_opt
                   | terminatable_statement ';'     newline_opt
                   ;


  unterminated_statement : terminatable_statement
                   | If '(' expr ')' newline_opt unterminated_statement
                   | If '(' expr ')' newline_opt terminated_statement
                        Else newline_opt unterminated_statement
                   | While '(' expr ')' newline_opt unterminated_statement
                   | For '(' simple_statement_opt ';'
                    expr_opt ';' simple_statement_opt ')' newline_opt
                        unterminated_statement
                   | For '(' NAME In NAME ')' newline_opt
                        unterminated_statement
                   ;


  terminatable_statement : simple_statement
                   | Break
                   | Continue
                   | Next
                   | Exit expr_opt
                   | Return expr_opt
                   | Do newline_opt terminated_statement While '(' expr ')'
                   ;


  simple_statement_opt : /* empty */
                   | simple_statement
                   ;


  simple_statement : Delete NAME '[' expr_list ']'
                   | expr
                   | print_statement
                   ;


  print_statement  : simple_print_statement
                   | simple_print_statement output_redirection
                   ;


  simple_print_statement : Print  print_expr_list_opt
                   | Print  '(' multiple_expr_list ')'
                   | Printf print_expr_list
                   | Printf '(' multiple_expr_list ')'
                   ;


  output_redirection : '>'    expr
                   | APPEND expr
                   | '|'    expr
                   ;


  expr_list_opt    : /* empty */
                   | expr_list
                   ;


  expr_list        : expr
                   | multiple_expr_list
                   ;


  multiple_expr_list : expr ',' newline_opt expr
                   | multiple_expr_list ',' newline_opt expr
                   ;


  expr_opt         : /* empty */
                   | expr
                   ;


  expr             : unary_expr
                   | non_unary_expr
                   ;


  unary_expr       : '+' expr
                   | '-' expr
                   | unary_expr '^'      expr
                   | unary_expr '*'      expr
                   | unary_expr '/'      expr
                   | unary_expr '%'      expr
                   | unary_expr '+'      expr
                   | unary_expr '-'      expr
                   | unary_expr          non_unary_expr
                   | unary_expr '<'      expr
                   | unary_expr LE       expr
                   | unary_expr NE       expr
                   | unary_expr EQ       expr
                   | unary_expr '>'      expr
                   | unary_expr GE       expr
                   | unary_expr ' '      expr
                   | unary_expr NO_MATCH expr
                   | unary_expr In NAME
                   | unary_expr AND newline_opt expr
                   | unary_expr OR  newline_opt expr
                   | unary_expr '?' expr ':' expr
                   | unary_input_function
                   ;


  non_unary_expr   : '(' expr ')'
                   | '!' expr
                   | non_unary_expr '^'      expr
                   | non_unary_expr '*'      expr
                   | non_unary_expr '/'      expr
                   | non_unary_expr '%'      expr
                   | non_unary_expr '+'      expr
                   | non_unary_expr '-'      expr
                   | non_unary_expr          non_unary_expr
                   | non_unary_expr '<'      expr
                   | non_unary_expr LE       expr
                   | non_unary_expr NE       expr
                   | non_unary_expr EQ       expr
                   | non_unary_expr '>'      expr
                   | non_unary_expr GE       expr
                   | non_unary_expr ' '      expr
                   | non_unary_expr NO_MATCH expr
                   | non_unary_expr In NAME
                   | '(' multiple_expr_list ')' In NAME
                   | non_unary_expr AND newline_opt expr
                   | non_unary_expr OR  newline_opt expr
                   | non_unary_expr '?' expr ':' expr
                   | NUMBER
                   | STRING
                   | lvalue
                   | ERE
                   | XPATH
                   | lvalue INCR
                   | lvalue DECR
                   | INCR lvalue
                   | DECR lvalue
                   | lvalue POW_ASSIGN expr
                   | lvalue MOD_ASSIGN expr
                   | lvalue MUL_ASSIGN expr
                   | lvalue DIV_ASSIGN expr
                   | lvalue ADD_ASSIGN expr
                   | lvalue SUB_ASSIGN expr
                   | lvalue '=' expr
                   | FUNC_NAME '(' expr_list_opt ')'
                        /* no white space allowed before '(' */
                   | BUILTIN_FUNC_NAME '(' expr_list_opt ')'
                   | BUILTIN_FUNC_NAME
                   | non_unary_input_function
                   ;


  print_expr_list_opt : /* empty */
                   | print_expr_list
                   ;


  print_expr_list  : print_expr
                   | print_expr_list ',' newline_opt print_expr
                   ;


  print_expr       : unary_print_expr
                   | non_unary_print_expr
                   ;


  unary_print_expr : '+' print_expr
                   | '-' print_expr
                   | unary_print_expr '^'      print_expr
                   | unary_print_expr '*'      print_expr
                   | unary_print_expr '/'      print_expr
                   | unary_print_expr '%'      print_expr
                   | unary_print_expr '+'      print_expr
                   | unary_print_expr '-'      print_expr
                   | unary_print_expr          non_unary_print_expr
                   | unary_print_expr ' '      print_expr
                   | unary_print_expr NO_MATCH print_expr
                   | unary_print_expr In NAME
                   | unary_print_expr AND newline_opt print_expr
                   | unary_print_expr OR  newline_opt print_expr
                   | unary_print_expr '?' print_expr ':' print_expr
                   ;


  non_unary_print_expr : '(' expr ')'
                   | '!' print_expr
                   | non_unary_print_expr '^'      print_expr
                   | non_unary_print_expr '*'      print_expr
                   | non_unary_print_expr '/'      print_expr
                   | non_unary_print_expr '%'      print_expr
                   | non_unary_print_expr '+'      print_expr
                   | non_unary_print_expr '-'      print_expr
                   | non_unary_print_expr          non_unary_print_expr
                   | non_unary_print_expr ' '      print_expr
                   | non_unary_print_expr NO_MATCH print_expr
                   | non_unary_print_expr In NAME
                   | '(' multiple_expr_list ')' In NAME
                   | non_unary_print_expr AND newline_opt print_expr
                   | non_unary_print_expr OR  newline_opt print_expr
                   | non_unary_print_expr '?' print_expr ':' print_expr
                   | NUMBER
                   | STRING
                   | lvalue
                   | ERE
                   | lvalue INCR
                   | lvalue DECR
                   | INCR lvalue
                   | DECR lvalue
                   | lvalue POW_ASSIGN print_expr
                   | lvalue MOD_ASSIGN print_expr
                   | lvalue MUL_ASSIGN print_expr
                   | lvalue DIV_ASSIGN print_expr
                   | lvalue ADD_ASSIGN print_expr
                   | lvalue SUB_ASSIGN print_expr
                   | lvalue '=' print_expr
                   | FUNC_NAME '(' expr_list_opt ')'
                       /* no white space allowed before '(' */
                   | BUILTIN_FUNC_NAME '(' expr_list_opt ')'
                   | BUILTIN_FUNC_NAME
                   ;


  lvalue           : NAME
                   | NAME '[' expr_list ']'
                   | '$' expr
                   ;


  non_unary_input_function : simple_get
                   | simple_get '<' expr
                   | non_unary_expr '|' simple_get
                   ;


  unary_input_function : unary_expr '|' simple_get
                   ;


  simple_get       : GETLINE
                   | GETLINE lvalue
                   ;


  newline_opt      : /* empty */
                   | newline_opt NEWLINE
                   ;

%%

bool_t parse_string_awkvm(awkvm_t *vm, const char_t *begin, const char_t *end) {
  int ok;
#if YYDEBUG
  yydebug = 1;
#endif

  avm = vm;

  reset_lexer_awk();
  scan_lexer_awk((char *)begin, end - begin);
  ok = yyparse();
  free_lexer_awk();

  avm = NULL;

  return (ok == 0);
}

bool_t parse_file_awkvm(awkvm_t *vm, const char *file) {
  int ok;
#if YYDEBUG
  yydebug = 1;
#endif
  yyin = fopen(file, "r");
  if( yyin ) {

    avm = vm;

    reset_lexer_awk();
    ok = yyparse();
    free_lexer_awk();
    fclose(yyin);
    yyin = NULL;

    avm = NULL;

    return (ok == 0);
  }
  return FALSE;
}

int yyerror(char *s)
{ 
  errormsg(E_FATAL, "%s: parse error at line %ld before '%s'\n", 
	   inputfile, inputline, yytext); 
  return 0;
}

/* we never parse multiple files */
int yywrap() { return 1; }
