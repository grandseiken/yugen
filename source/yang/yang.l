%{

#include "yang.y.h"
#include "../../source/common/algorithm.h"
#include "../../source/yang/ast.h"
#include "../../source/yang/pipeline.h"

  /* Handle lexer input from a string. */

#undef YY_INPUT
#define YY_INPUT(b, r, m) lexer_input(b, &r, m)

namespace {

  int lexer_input(char* buffer, y::size* bytes_read, y::size bytes_max)
  {
    const y::string& in = *ParseGlobals::lexer_input_contents;
    y::size& offset = ParseGlobals::lexer_input_offset;

    *bytes_read = y::min(bytes_max, in.length() - offset);
    for (y::size i = 0; i < *bytes_read; ++i) {
      buffer[i] = in[i + offset];
    }
    offset += *bytes_read;
    return 0;
  }

}

%}

%option noyywrap
%x long_comment

  /* Regular expressions. */

newline \n|\r|\r\n
whitespace [ ]+|{newline}
comment \/\/.*{newline}
long_comment_begin \/\*
long_comment_end \*\/
long_comment_consume \*|[^*]+
default .|\n|\r

alpha [A-Za-z_]
digit [0-9]
identifier {alpha}({alpha}|{digit})*
int_literal {digit}+
world_literal \.{digit}+|{digit}+\.({digit}+)?

%%

  /* Comments and whitespace. */

<INITIAL>{long_comment_begin} {BEGIN(long_comment);}
<long_comment>{long_comment_end} {BEGIN(INITIAL);}
<long_comment>{long_comment_consume} {}
{whitespace}|{comment} {}

  /* Keywords. */

int {return T_INT;}
world {return T_WORLD;}
if {return T_IF;}
else {return T_ELSE;}
for {return T_FOR;}
global {return T_GLOBAL;}
export {return T_EXPORT;}

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
"." {return T_WORLD_CAST;}

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

  /* Identifiers and literals. */

true {
  yylval.node = new Node(Node::INT_LITERAL, 1);
  return T_INT_LITERAL;
}
false {
  yylval.node = new Node(Node::INT_LITERAL, 0);
  return T_INT_LITERAL;
}
{identifier} {
  yylval.node = new Node(Node::IDENTIFIER, yytext);
  return T_IDENTIFIER;
}
{int_literal} {
  yylval.node = new Node(Node::INT_LITERAL, y::stoi(yytext));
  return T_INT_LITERAL;
}
{world_literal} {
  yylval.node = new Node(Node::WORLD_LITERAL, y::stod(yytext));
  return T_WORLD_LITERAL;
}

  /* Default. */

{default} {return yytext[0];}
