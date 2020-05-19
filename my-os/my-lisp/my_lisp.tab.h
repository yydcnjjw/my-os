/* A Bison parser, made by GNU Bison 3.5.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_HOME_YYDCNJJW_WORKSPACE_CODE_PROJECT_MY_LISP_DEBUG_SRC_MY_LISP_TAB_H_INCLUDED
# define YY_YY_HOME_YYDCNJJW_WORKSPACE_CODE_PROJECT_MY_LISP_DEBUG_SRC_MY_LISP_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 10 "my_lisp.y"

    typedef void* yyscan_t;
    #include "my_lisp.h"

#line 53 "/home/yydcnjjw/workspace/code/project/my-lisp/debug/src/my_lisp.tab.h"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    LP = 258,
    RP = 259,
    LSB = 260,
    RSB = 261,
    APOSTROPHE = 262,
    GRAVE = 263,
    COMMA = 264,
    COMMA_AT = 265,
    PERIOD = 266,
    NS_APOSTROPHE = 267,
    NS_GRAVE = 268,
    NS_COMMA = 269,
    NS_COMMA_AT = 270,
    VECTOR_LP = 271,
    VECTOR_BYTE_LP = 272,
    IDENTIFIER = 273,
    BOOLEAN_T = 274,
    BOOLEAN_F = 275,
    NUMBER = 276,
    CHARACTER = 277,
    STRING = 278,
    END_OF_FILE = 279
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 19 "my_lisp.y"

    number *num;
    symbol *symbol;
    object *obj;
    string *str;
    const char *abbrev;
    u16 ch;

#line 98 "/home/yydcnjjw/workspace/code/project/my-lisp/debug/src/my_lisp.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int yyparse (yyscan_t scanner, parse_data *data);

#endif /* !YY_YY_HOME_YYDCNJJW_WORKSPACE_CODE_PROJECT_MY_LISP_DEBUG_SRC_MY_LISP_TAB_H_INCLUDED  */
