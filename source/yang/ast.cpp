#include "ast.h"

#include "../log.h"
#include "../../gen/yang/yang.y.h"
#include "../../gen/yang/yang.l.h"

int yyparse();

y::unique<Node> parse_yang_ast(const y::string& contents)
{
  ParseGlobals::lexer_input_contents = &contents;
  ParseGlobals::lexer_input_offset = 0;
  ParseGlobals::lexer_line = 0;
  ParseGlobals::errors.clear();

  yyparse();
  for (const y::string& s : ParseGlobals::errors) {
    log_err(s);
  }
  if (ParseGlobals::errors.size()) {
    return y::move_unique<Node>(y::null);
  }
  return y::move_unique(ParseGlobals::parser_output);
}

Node::Node(node_type type)
  : type(type)
  , int_value(0)
  , world_value(0)
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
{
}

Node::Node(node_type type, y::world value)
  : type(type)
  , int_value(0)
  , world_value(value)
{
}

Node::Node(node_type type, y::string value)
  : type(type)
  , int_value(0)
  , world_value(0)
  , string_value(value)
{
}

void Node::add(Node* node)
{
  children.push_back(y::move_unique(node));
}

const y::string* ParseGlobals::lexer_input_contents = y::null;
y::size ParseGlobals::lexer_input_offset = 0;
y::size ParseGlobals::lexer_line = 0;

Node* ParseGlobals::parser_output = y::null;
y::vector<y::string> ParseGlobals::errors;
