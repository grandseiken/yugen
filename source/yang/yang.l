%{

#include "yang.y.h"

%}

%option noyywrap
%x long_comment

  /* Regular expressions. */

whitespace [ ]+
newline \n|\r|\r\n
comment \/\/.*{newline}
long_comment_begin \/\*
long_comment_end \*\/
long_comment_consume \*|[^*\n\r]+
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
<long_comment>{newline} {++yyline;}
<long_comment>{long_comment_consume} {}
{whitespace} {}
{comment}|{newline} {++yyline;}

  /* Keywords. */

int {return T_INT;}
world {return T_WORLD;}
if {return T_IF;}
else {return T_ELSE;}
for {return T_FOR;}
global {return T_GLOBAL;}
parameter {return T_PARAMETER;}

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

{identifier} {yylval.string_value = yytext; return T_IDENTIFIER;}
{int_literal} {yylval.int_value = y::stoi(yytext); return T_INT_LITERAL;}
{world_literal} {yylval.world_value = y::stod(yytext); return T_WORLD_LITERAL;}

  /* Default. */

{default} {return yytext[0];}
