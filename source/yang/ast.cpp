#include "ast.h"

Node::Node()
  : int_value(0)
  , world_value(0)
{
}

Node::Node(Node* a)
  : Node()
{
  add(a);
}

Node::Node(Node* a, Node* b)
{
  add(a);
  add(b);
}

Node::Node(Node* a, Node* b, Node* c)
{
  add(a);
  add(b);
  add(c);
}

Node::Node(y::int32 value)
  : int_value(value)
  , world_value(0)
{
}

Node::Node(y::world value)
  : int_value(0)
  , world_value(value)
{
}

Node::Node(y::string value)
  : int_value(0)
  , world_value(0)
  , string_value(value)
{
}

void Node::add(Node* node)
{
  children.push_back(y::move_unique(node));
}
