#include "ast.h"
#include "../../gen/yang/yang.l.h"

Node::Node(node_type type)
  : type(type)
  , int_value(0)
  , world_value(0)
  , line(yylineno)
  , text(yytext)
{
}

Node::Node(node_type type, Node* a)
  : Node(type)
{
  add(a);
}

Node::Node(node_type type, Node* a, Node* b)
  : Node(type)
{
  add(a);
  add(b);
}

Node::Node(node_type type, Node* a, Node* b, Node* c)
  : Node(type)
{
  add(a);
  add(b);
  add(c);
}

Node::Node(node_type type, y::int32 value)
  : type(type)
  , int_value(value)
  , world_value(0)
  , line(yylineno)
  , text(yytext)
{
}

Node::Node(node_type type, y::world value)
  : type(type)
  , int_value(0)
  , world_value(value)
  , line(yylineno)
  , text(yytext)
{
}

Node::Node(node_type type, y::string value)
  : type(type)
  , int_value(0)
  , world_value(0)
  , string_value(value)
  , line(yylineno)
  , text(yytext)
{
}

void Node::add(Node* node)
{
  children.push_back(y::move_unique(node));
}

y::string Node::op_string(node_type t)
{
  y::string s =
      t == Node::TERNARY ? "?:" :
      t == Node::LOGICAL_OR ? "||" :
      t == Node::LOGICAL_AND ? "&&" :
      t == Node::BITWISE_OR ? "|" :
      t == Node::BITWISE_AND ? "&" :
      t == Node::BITWISE_XOR ? "^" :
      t == Node::BITWISE_LSHIFT ? "<<" :
      t == Node::BITWISE_RSHIFT ? ">>" :
      t == Node::POW ? "**" :
      t == Node::MOD ? "%" :
      t == Node::ADD ? "+" :
      t == Node::SUB ? "-" :
      t == Node::MUL ? "*" :
      t == Node::DIV ? "/" :
      t == Node::EQ ? "==" :
      t == Node::NE ? "!=" :
      t == Node::GE ? ">=" :
      t == Node::LE ? "<=" :
      t == Node::GT ? ">" :
      t == Node::LT ? "<" :
      t == Node::LOGICAL_NEGATION ? "!" :
      t == Node::BITWISE_NEGATION ? "~" :
      t == Node::ARITHMETIC_NEGATION ? "-" :
      t == Node::INT_CAST ? "[]" :
      t == Node::WORLD_CAST ? "." :
      "unknown operator";
  return "`" + s + "`";
}
