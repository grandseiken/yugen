%{

#include "yang.l.h"
#include "../../source/yang/ast.h"
#include "../../source/yang/pipeline.h"

namespace {

  int yyerror(const char* message)
  {
    y::sstream ss;
    ss << "Error at line " << yylineno <<
        ", near`" << yytext << "`:\n\t" << message;
    ParseGlobals::errors.emplace_back(ss.str());
    return 0;
  }

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
%token T_EXPORT

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
%token T_WORLD_CAST
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
%token <node> T_IDENTIFIER
%token <node> T_INT_LITERAL
%token <node> T_WORLD_LITERAL

  /* Operator precedence. */

%left T_TERNARY
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
%left T_WORLD_CAST

  /* Types. */

%type <node> program
%type <node> expr
%type <node> t_expr
%start program

%%

  /* Expressions. */

program
  : expr
{$$ = ParseGlobals::parser_output = $1;}
  ;

expr
  : t_expr T_TERNARY_L expr T_TERNARY_R expr
{$$ = new Node(Node::TERNARY, $1, $3, $5);}
  | t_expr
{$$ = $1;}
  ;

t_expr
  : '(' expr ')'
{$$ = $2;}
  | T_IDENTIFIER
{$$ = $1;}
  | T_INT_LITERAL
{$$ = $1;}
  | T_WORLD_LITERAL
{$$ = $1;}
  | t_expr T_LOGICAL_OR t_expr
{$$ = new Node(Node::LOGICAL_OR, $1, $3);}
  | t_expr T_LOGICAL_AND t_expr
{$$ = new Node(Node::LOGICAL_AND, $1, $3);}
  | t_expr T_BITWISE_OR t_expr
{$$ = new Node(Node::BITWISE_OR, $1, $3);}
  | t_expr T_BITWISE_AND t_expr
{$$ = new Node(Node::BITWISE_AND, $1, $3);}
  | t_expr T_BITWISE_XOR t_expr
{$$ = new Node(Node::BITWISE_XOR, $1, $3);}
  | t_expr T_BITWISE_LSHIFT t_expr
{$$ = new Node(Node::BITWISE_LSHIFT, $1, $3);}
  | t_expr T_BITWISE_RSHIFT t_expr
{$$ = new Node(Node::BITWISE_RSHIFT, $1, $3);}
  | t_expr T_POW t_expr
{$$ = new Node(Node::POW, $1, $3);}
  | t_expr T_MOD t_expr
{$$ = new Node(Node::MOD, $1, $3);}
  | t_expr T_ADD t_expr
{$$ = new Node(Node::ADD, $1, $3);}
  | t_expr T_SUB t_expr
{$$ = new Node(Node::SUB, $1, $3);}
  | t_expr T_MUL t_expr
{$$ = new Node(Node::MUL, $1, $3);}
  | t_expr T_DIV t_expr
{$$ = new Node(Node::DIV, $1, $3);}
  | t_expr T_EQ t_expr
{$$ = new Node(Node::EQ, $1, $3);}
  | t_expr T_NE t_expr
{$$ = new Node(Node::NE, $1, $3);}
  | t_expr T_GE t_expr
{$$ = new Node(Node::GE, $1, $3);}
  | t_expr T_LE t_expr
{$$ = new Node(Node::LE, $1, $3);}
  | t_expr T_GT t_expr
{$$ = new Node(Node::GT, $1, $3);}
  | t_expr T_LT t_expr
{$$ = new Node(Node::LT, $1, $3);}
  | T_LOGICAL_NEGATION t_expr %prec T_UNARY
{$$ = new Node(Node::LOGICAL_NEGATION, $2);}
  | T_BITWISE_NEGATION t_expr %prec T_UNARY
{$$ = new Node(Node::BITWISE_NEGATION, $2);}
  | T_SUB t_expr %prec T_UNARY
{$$ = new Node(Node::ARITHMETIC_NEGATION, $2);}
  | '[' expr ']'
{$$ = new Node(Node::INT_CAST, $2);}
  | t_expr T_WORLD_CAST
{$$ = new Node(Node::WORLD_CAST, $1);}
  ;
