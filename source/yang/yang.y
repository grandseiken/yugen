%{

#include "../../source/yang/ast.h"

extern int yylex();
int yyerror(const char* message)
{
  return 0;
}

%}

%union {
  struct Node* node;
}

  /* Tokens. */

%token T_INT
%token T_WORLD
%token T_IF
%token T_ELSE
%token T_FOR
%token T_GLOBAL
%token T_PARAMETER
%token T_TERNARY_L
%token T_TERNARY_R
%token T_LOGICAL_NEGATION
%token T_LOGICAL_OR
%token T_LOGICAL_AND
%token T_BITWISE_NEGATION
%token T_BITWISE_OR
%token T_BITWISE_AND
%token T_BITWISE_XOR
%token T_BITWISE_LSHIFT
%token T_BITWISE_RSHIFT
%token T_POW
%token T_MOD
%token T_ADD
%token T_SUB
%token T_MUL
%token T_DIV
%token T_EQ
%token T_NE
%token T_GE
%token T_LE
%token T_GT
%token T_LT
%token T_ASSIGN
%token T_ASSIGN_LOGICAL_OR
%token T_ASSIGN_LOGICAL_AND
%token T_ASSIGN_BITWISE_OR
%token T_ASSIGN_BITWISE_AND
%token T_ASSIGN_BITWISE_XOR
%token T_ASSIGN_BITWISE_LSHIFT
%token T_ASSIGN_BITWISE_RSHIFT
%token T_ASSIGN_POW
%token T_ASSIGN_MOD
%token T_ASSIGN_ADD
%token T_ASSIGN_SUB
%token T_ASSIGN_MUL
%token T_ASSIGN_DIV
%token <string_value> T_IDENTIFIER
%token <int_value> T_INT_LITERAL
%token <world_value> T_WORLD_LITERAL

  /* Operator precedence. */

%left T_TERNARY_L T_TERNARY_R
%left T_LOGICAL_OR
%left T_LOGICAL_AND
%left T_EQ T_NE
%left T_GE T_LE T_GT T_LT
%left T_BITWISE_OR
%left T_BITWISE_XOR
%left T_BITWISE_AND
%left T_BITWISE_LSHIFT T_BITWISE_RSHIFT
%left T_ADD T_SUB
%left T_MUL T_DIV T_MOD
%right T_UNARY
%left T_POW

  /* Types. */

%start expr

%%

  /* Expressions. */

expr
  : t_expr T_TERNARY_L expr T_TERNARY_R expr
  | t_expr
  ;

t_expr
  : T_IDENTIFIER
  | T_INT_LITERAL
  | T_WORLD_LITERAL
  | t_expr T_LOGICAL_OR t_expr
  | t_expr T_LOGICAL_AND t_expr
  | t_expr T_BITWISE_OR t_expr
  | t_expr T_BITWISE_AND t_expr
  | t_expr T_BITWISE_XOR t_expr
  | t_expr T_BITWISE_LSHIFT t_expr
  | t_expr T_BITWISE_RSHIFT t_expr
  | t_expr T_POW t_expr
  | t_expr T_MOD t_expr
  | t_expr T_ADD t_expr
  | t_expr T_SUB t_expr
  | t_expr T_MUL t_expr
  | t_expr T_DIV t_expr
  | t_expr T_EQ t_expr
  | t_expr T_NE t_expr
  | t_expr T_GE t_expr
  | t_expr T_LE t_expr
  | t_expr T_GT t_expr
  | t_expr T_LT t_expr
  | T_LOGICAL_NEGATION t_expr %prec T_UNARY
  | T_BITWISE_NEGATION t_expr %prec T_UNARY
  | T_SUB t_expr %prec T_UNARY
  ;
