%{

#include "../../src/common/algorithm.h"
#include "../../src/yang/ast.h"
#include "yang.y.h"

  /* Handle lexer input from a string. */

#undef YY_INPUT
#define YY_INPUT(b, r, m) lexer_input(b, &r, m)
int yang_error(const char* message);

namespace {

  int lexer_input(char* buffer, y::size* bytes_read, y::size bytes_max)
  {
    const y::string& in = *yang::ParseGlobals::lexer_input_contents;
    y::size& offset = yang::ParseGlobals::lexer_input_offset;

    *bytes_read = y::min(bytes_max, in.length() - offset);
    for (y::size i = 0; i < *bytes_read; ++i) {
      buffer[i] = in[i + offset];
    }
    offset += *bytes_read;
    return 0;
  }

}

typedef yang::Node Node;

%}

%option noyywrap
%option yylineno
%x line_comment
%x long_comment

  /* Regular expressions. */

newline \n|\r|\r\n
whitespace [ ]+|{newline}
whitespace_newline [ ]+{newline}
tab \t
line_comment_begin \/\/
long_comment_begin \/\*
long_comment_end \*\/
default .|\n|\r

alpha [A-Za-z_]
digit [0-9]
identifier {alpha}({alpha}|{digit})*
int_literal {digit}+
world_literal \.{digit}+|{digit}+\.({digit}+)?
vector_size [1-9]{digit}*

%%

  /* Comments and whitespace. */

<*>{tab} {
  yang_error("tab character");
}
<*>{whitespace_newline} {
  yang_error("end-of-line whitespace");
}
<long_comment><<EOF>> {
  yang_error("end-of-input inside comment");
  BEGIN(INITIAL);
  return T_EOF;
}
<INITIAL>{line_comment_begin} {BEGIN(line_comment);}
<INITIAL>{long_comment_begin} {BEGIN(long_comment);}
<line_comment>{newline} {BEGIN(INITIAL);}
<long_comment>{long_comment_end} {BEGIN(INITIAL);}
<line_comment,long_comment>{default} {}
{whitespace} {}

  /* Keywords. */

global {return T_GLOBAL;}
export {return T_EXPORT;}
var {return T_VAR;}
const {return T_CONST;}
if {return T_IF;}
else {return T_ELSE;}
for {return T_FOR;}
while {return T_WHILE;}
do {return T_DO;}
break {return T_BREAK;}
continue {return T_CONTINUE;}
return {return T_RETURN;}

  /* Types. */

void {
  yang_lval.node = new Node(Node::TYPE_VOID);
  return T_TYPE_LITERAL;
}
int {
  yang_lval.node = new Node(Node::TYPE_INT, 1);
  return T_TYPE_LITERAL;
}
int0 {
  yang_lval.node = new Node(Node::TYPE_VOID);
  return T_TYPE_LITERAL;
}
int{vector_size} {
  yang_lval.node = new Node(Node::TYPE_INT, y::stoi(3 + yang_text));
  return T_TYPE_LITERAL;
}
world {
  yang_lval.node = new Node(Node::TYPE_WORLD, 1);
  return T_TYPE_LITERAL;
}
world0 {
  yang_lval.node = new Node(Node::TYPE_VOID);
  return T_TYPE_LITERAL;
}
world{vector_size} {
  yang_lval.node = new Node(Node::TYPE_WORLD, y::stoi(5 + yang_text));
  return T_TYPE_LITERAL;
}

  /* Operators. */

"?" {return T_TERNARY_L;}
":" {return T_TERNARY_R;}
"!" {return T_LOGICAL_NEGATION;}
"||" {return T_LOGICAL_OR;}
"&&" {return T_LOGICAL_AND;}
"~" {return T_BITWISE_NEGATION;}
"|" {return T_BITWISE_OR;}
"&" {return T_BITWISE_AND;}
"^" {return T_BITWISE_XOR;}
"<<" {return T_BITWISE_LSHIFT;}
">>" {return T_BITWISE_RSHIFT;}
"**" {return T_POW;}
"%" {return T_MOD;}
"+" {return T_ADD;}
"-" {return T_SUB;}
"*" {return T_MUL;}
"/" {return T_DIV;}
"==" {return T_EQ;}
"!=" {return T_NE;}
">=" {return T_GE;}
"<=" {return T_LE;}
">" {return T_GT;}
"<" {return T_LT;}
"$" {return T_FOLD;}
"." {return T_WORLD_CAST;}

  /* Assignment operators. */

"=" {return T_ASSIGN;}
"||=" {return T_ASSIGN_LOGICAL_OR;}
"&&=" {return T_ASSIGN_LOGICAL_AND;}
"|=" {return T_ASSIGN_BITWISE_OR;}
"&=" {return T_ASSIGN_BITWISE_AND;}
"^=" {return T_ASSIGN_BITWISE_XOR;}
"<<=" {return T_ASSIGN_BITWISE_LSHIFT;}
">>=" {return T_ASSIGN_BITWISE_RSHIFT;}
"**=" {return T_ASSIGN_POW;}
"%=" {return T_ASSIGN_MOD;}
"+=" {return T_ASSIGN_ADD;}
"-=" {return T_ASSIGN_SUB;}
"*=" {return T_ASSIGN_MUL;}
"/=" {return T_ASSIGN_DIV;}
"++" {return T_INCREMENT;}
"--" {return T_DECREMENT;}

  /* Identifiers and literals. */

true {
  yang_lval.node = new Node(Node::INT_LITERAL, 1);
  return T_INT_LITERAL;
}
false {
  yang_lval.node = new Node(Node::INT_LITERAL, 0);
  return T_INT_LITERAL;
}
{identifier} {
  yang_lval.node = new Node(Node::IDENTIFIER, yang_text);
  return T_IDENTIFIER;
}
{int_literal} {
  yang_lval.node = new Node(Node::INT_LITERAL, y::stoi(yang_text));
  return T_INT_LITERAL;
}
{world_literal} {
  yang_lval.node = new Node(Node::WORLD_LITERAL, y::stod(yang_text));
  return T_WORLD_LITERAL;
}

  /* Default. */

{default} {return yang_text[0];}