#ifndef YANG__AST_H
#define YANG__AST_H

#include "../common/math.h"
#include "../common/memory.h"
#include "../common/string.h"
#include "../common/vector.h"

// Child nodes passed to constructors or add transfer ownership, and are
// destroyed when the parent is destroyed.
struct Node {
  Node();
  Node(Node* a);
  Node(Node* a, Node* b);
  Node(Node* a, Node* b, Node* c);

  Node(y::int32 value);
  Node(y::world value);
  Node(y::string value);

  void add(Node* node);

  y::int32 int_value;
  y::world world_value;
  y::string string_value;

  y::vector<y::unique<Node>> children;
};

#endif
