#include "ast.h"
#include "../../gen/yang/yang.l.h"

Node::Node(node_type type)
  : line(yylineno)
  , text(yytext ? yytext : "")
  , type(type)
  , int_value(0)
  , world_value(0)
{
  orphans.insert(this);
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
  : line(yylineno)
  , text(yytext ? yytext : "")
  , type(type)
  , int_value(value)
  , world_value(0)
{
  orphans.insert(this);
}

Node::Node(node_type type, y::world value)
  : line(yylineno)
  , text(yytext ? yytext : "")
  , type(type)
  , int_value(0)
  , world_value(value)
{
  orphans.insert(this);
}

Node::Node(node_type type, y::string value)
  : line(yylineno)
  , text(yytext ? yytext : "")
  , type(type)
  , int_value(0)
  , world_value(0)
  , string_value(value)
{
  orphans.insert(this);
}

void Node::add_front(Node* node)
{
  orphans.erase(node);
  children.insert(children.begin(), y::move_unique(node));
}

void Node::add(Node* node)
{
  orphans.erase(node);
  children.push_back(y::move_unique(node));
}

y::string Node::op_string(node_type t)
{
  return
      t == Node::TERNARY ? "?:" :
      t == Node::CALL ? "()" :
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
      t == Node::FOLD_LOGICAL_OR ? "||" :
      t == Node::FOLD_LOGICAL_AND ? "&&" :
      t == Node::FOLD_BITWISE_OR ? "|" :
      t == Node::FOLD_BITWISE_AND ? "&" :
      t == Node::FOLD_BITWISE_XOR ? "^" :
      t == Node::FOLD_BITWISE_LSHIFT ? "<<" :
      t == Node::FOLD_BITWISE_RSHIFT ? ">>" :
      t == Node::FOLD_POW ? "**" :
      t == Node::FOLD_MOD ? "%" :
      t == Node::FOLD_ADD ? "+" :
      t == Node::FOLD_SUB ? "-" :
      t == Node::FOLD_MUL ? "*" :
      t == Node::FOLD_DIV ? "/" :
      t == Node::FOLD_EQ ? "==" :
      t == Node::FOLD_NE ? "!=" :
      t == Node::FOLD_GE ? ">=" :
      t == Node::FOLD_LE ? "<=" :
      t == Node::FOLD_GT ? ">" :
      t == Node::FOLD_LT ? "<" :
      t == Node::LOGICAL_NEGATION ? "!" :
      t == Node::BITWISE_NEGATION ? "~" :
      t == Node::ARITHMETIC_NEGATION ? "-" :
      t == Node::INT_CAST ? "[]" :
      t == Node::WORLD_CAST ? "." :
      t == Node::VECTOR_CONSTRUCT ? "()" :
      t == Node::VECTOR_INDEX ? "[]" :
      "unknown operator";
}

y::set<Node*> Node::orphans;
