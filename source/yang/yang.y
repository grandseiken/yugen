%{

#include "yang.l.h"
#include "../../source/yang/ast.h"
#include "../../source/yang/pipeline.h"

int yyerror(const char* message)
{
  ParseGlobals::errors.emplace_back(
      ParseGlobals::error(yylineno, yytext, message));
  return 0;
}

%}

%union {
  struct Node* node;
}

  /* Tokens. */

%token T_EOF 0

%token T_VAR
%token T_INT
%token T_WORLD
%token T_IF
%token T_ELSE
%token T_FOR
%token T_RETURN
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
%token T_FOLD
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
%token T_INCREMENT
%token T_DECREMENT
%token <node> T_IDENTIFIER
%token <node> T_INT_LITERAL
%token <node> T_WORLD_LITERAL

  /* Operator precedence. */

%nonassoc T_IF
%nonassoc T_ELSE
%right T_TERNARY_L T_TERNARY_R T_ASSIGN
  /* TODO: Precedence for fold doesn't work right.
     For example: $(1, 1) == (1, 1)&& parses but doesn't with bitwise &.
     Depends on relative precedence of infix operator with fold operator.
     Doubtful whether this is fixable, but maybe some clever alternate syntax
     is possible? */
%left T_FOLD
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
%right P_UNARY_L
%left T_POW
%left T_WORLD_CAST '['

  /* Types. */

%type <node> program
%type <node> elem_list
%type <node> elem
%type <node> function
%type <node> stmt_list
%type <node> stmt
%type <node> expr_list
%type <node> expr
%start program

%%

  /* Language grammar. */

program
  : elem_list T_EOF
{$$ = ParseGlobals::parser_output = $1;
 $$->type = Node::PROGRAM;}
  ;

elem_list
  :
{$$ = new Node(Node::ERROR);}
  | elem_list elem
{$$ = $1; $$->add($2);}
  ;

elem
  : function
{$$ = $1;}
  ;

function
  : T_IDENTIFIER '(' ')' stmt
{$$ = new Node(Node::FUNCTION, $4);
 $$->string_value = $1->string_value;}
  ;

stmt_list
  :
{$$ = new Node(Node::ERROR);}
  | stmt_list stmt
{$$ = $1; $$->add($2);}

stmt
  : ';'
{$$ = new Node(Node::EMPTY_STMT);}
  | expr ';'
{$$ = new Node(Node::EXPR_STMT, $1);}
  | T_RETURN expr ';'
{$$ = new Node(Node::RETURN_STMT, $2);}
  | T_IF '(' expr ')' stmt %prec T_IF
{$$ = new Node(Node::IF_STMT, $3, $5);}
  | T_IF '(' expr ')' stmt T_ELSE stmt
{$$ = new Node(Node::IF_STMT, $3, $5, $7);}
  | T_WHILE '(' expr ')' stmt
{$$ = new Node(Node::WHILE_STMT, $3, $5);}
  | '{' stmt_list '}'
{$$ = $2; $$->type = Node::BLOCK;}
  ;

expr_list
  : expr
{$$ = new Node(Node::ERROR, $1);}
  | expr_list ',' expr
{$$ = $1; $$->add($3);}
  ;

expr
  /* Miscellaeneous. */
  : '(' expr ')'
{$$ = $2;}
  | expr T_TERNARY_L expr T_TERNARY_R expr
{$$ = new Node(Node::TERNARY, $1, $3, $5);}
  | T_IDENTIFIER
{$$ = $1;}
  | T_INT_LITERAL
{$$ = $1;}
  | T_WORLD_LITERAL
{$$ = $1;}
  /* Binary operators. */
  | expr T_LOGICAL_OR expr
{$$ = new Node(Node::LOGICAL_OR, $1, $3);}
  | expr T_LOGICAL_AND expr
{$$ = new Node(Node::LOGICAL_AND, $1, $3);}
  | expr T_BITWISE_OR expr
{$$ = new Node(Node::BITWISE_OR, $1, $3);}
  | expr T_BITWISE_AND expr
{$$ = new Node(Node::BITWISE_AND, $1, $3);}
  | expr T_BITWISE_XOR expr
{$$ = new Node(Node::BITWISE_XOR, $1, $3);}
  | expr T_BITWISE_LSHIFT expr
{$$ = new Node(Node::BITWISE_LSHIFT, $1, $3);}
  | expr T_BITWISE_RSHIFT expr
{$$ = new Node(Node::BITWISE_RSHIFT, $1, $3);}
  | expr T_POW expr
{$$ = new Node(Node::POW, $1, $3);}
  | expr T_MOD expr
{$$ = new Node(Node::MOD, $1, $3);}
  | expr T_ADD expr
{$$ = new Node(Node::ADD, $1, $3);}
  | expr T_SUB expr
{$$ = new Node(Node::SUB, $1, $3);}
  | expr T_MUL expr
{$$ = new Node(Node::MUL, $1, $3);}
  | expr T_DIV expr
{$$ = new Node(Node::DIV, $1, $3);}
  | expr T_EQ expr
{$$ = new Node(Node::EQ, $1, $3);}
  | expr T_NE expr
{$$ = new Node(Node::NE, $1, $3);}
  | expr T_GE expr
{$$ = new Node(Node::GE, $1, $3);}
  | expr T_LE expr
{$$ = new Node(Node::LE, $1, $3);}
  | expr T_GT expr
{$$ = new Node(Node::GT, $1, $3);}
  | expr T_LT expr
{$$ = new Node(Node::LT, $1, $3);}
  /* Fold operators. */
  | T_FOLD expr T_LOGICAL_OR
{$$ = new Node(Node::FOLD_LOGICAL_OR, $2);}
  | T_FOLD expr T_LOGICAL_AND
{$$ = new Node(Node::FOLD_LOGICAL_AND, $2);}
  | T_FOLD expr T_BITWISE_OR
{$$ = new Node(Node::FOLD_BITWISE_OR, $2);}
  | T_FOLD expr T_BITWISE_AND
{$$ = new Node(Node::FOLD_BITWISE_AND, $2);}
  | T_FOLD expr T_BITWISE_XOR
{$$ = new Node(Node::FOLD_BITWISE_XOR, $2);}
  | T_FOLD expr T_BITWISE_LSHIFT
{$$ = new Node(Node::FOLD_BITWISE_LSHIFT, $2);}
  | T_FOLD expr T_BITWISE_RSHIFT
{$$ = new Node(Node::FOLD_BITWISE_RSHIFT, $2);}
  | T_FOLD expr T_POW
{$$ = new Node(Node::FOLD_POW, $2);}
  | T_FOLD expr T_MOD
{$$ = new Node(Node::FOLD_MOD, $2);}
  | T_FOLD expr T_ADD
{$$ = new Node(Node::FOLD_ADD, $2);}
  | T_FOLD expr T_SUB
{$$ = new Node(Node::FOLD_SUB, $2);}
  | T_FOLD expr T_MUL
{$$ = new Node(Node::FOLD_MUL, $2);}
  | T_FOLD expr T_DIV
{$$ = new Node(Node::FOLD_DIV, $2);}
  | T_FOLD expr T_EQ
{$$ = new Node(Node::FOLD_EQ, $2);}
  | T_FOLD expr T_NE
{$$ = new Node(Node::FOLD_NE, $2);}
  | T_FOLD expr T_GE
{$$ = new Node(Node::FOLD_GE, $2);}
  | T_FOLD expr T_LE
{$$ = new Node(Node::FOLD_LE, $2);}
  | T_FOLD expr T_GT
{$$ = new Node(Node::FOLD_GT, $2);}
  | T_FOLD expr T_LT
{$$ = new Node(Node::FOLD_LT, $2);}
  /* Prefix unary operators. */
  | T_LOGICAL_NEGATION expr %prec P_UNARY_L
{$$ = new Node(Node::LOGICAL_NEGATION, $2);}
  | T_BITWISE_NEGATION expr %prec P_UNARY_L
{$$ = new Node(Node::BITWISE_NEGATION, $2);}
  | T_SUB expr %prec P_UNARY_L
{$$ = new Node(Node::ARITHMETIC_NEGATION, $2);}
  /* Assignment operators. */
  | T_VAR T_IDENTIFIER T_ASSIGN expr
{$$ = new Node(Node::ASSIGN_VAR, $4);
 $$->string_value = $2->string_value;}
  | T_IDENTIFIER T_ASSIGN expr
{$$ = new Node(Node::ASSIGN, $3);
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_LOGICAL_OR expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::LOGICAL_OR, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_LOGICAL_AND expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::LOGICAL_AND, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_BITWISE_OR expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::BITWISE_OR, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_BITWISE_AND expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::BITWISE_AND, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_BITWISE_XOR expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::BITWISE_XOR, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_BITWISE_LSHIFT expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::BITWISE_LSHIFT, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_BITWISE_RSHIFT expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::BITWISE_RSHIFT, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_POW expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::POW, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_MOD expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::MOD, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_ADD expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::ADD, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_SUB expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::SUB, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_MUL expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::MUL, $1, $3));
 $$->string_value = $1->string_value;}
  | T_IDENTIFIER T_ASSIGN_DIV expr %prec T_ASSIGN
{$$ = new Node(Node::ASSIGN, new Node(Node::DIV, $1, $3));
 $$->string_value = $1->string_value;}
  | T_INCREMENT T_IDENTIFIER %prec P_UNARY_L
{$$ = new Node(Node::ASSIGN,
               new Node(Node::ADD, $2, new Node(Node::INT_LITERAL, 1)));
 $$->string_value = $2->string_value;}
  | T_DECREMENT T_IDENTIFIER %prec P_UNARY_L
{$$ = new Node(Node::ASSIGN,
               new Node(Node::SUB, $2, new Node(Node::INT_LITERAL, 1)));
 $$->string_value = $2->string_value;}
  /* Type-conversion operators. */
  | '[' expr ']'
{$$ = new Node(Node::INT_CAST, $2);}
  | expr T_WORLD_CAST
{$$ = new Node(Node::WORLD_CAST, $1);}
  | '(' expr ',' expr_list ')'
{$$ = $4; $$->add_front($2);
 $$->type = Node::VECTOR_CONSTRUCT;}
  | expr '[' expr ']'
{$$ = new Node(Node::VECTOR_INDEX, $1, $3);}
  ;

  /* Error-handling. */

