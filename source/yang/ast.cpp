#include "ast.h"

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
