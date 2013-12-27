#ifndef YANG__AST_H
#define YANG__AST_H

#include "../common/math.h"
#include "../common/memory.h"
#include "../common/set.h"
#include "../common/string.h"
#include "../common/vector.h"

struct Node {
  enum node_type {
    IDENTIFIER,
    INT_LITERAL,
    WORLD_LITERAL,
    TERNARY,
    LOGICAL_OR,
    LOGICAL_AND,
    BITWISE_OR,
    BITWISE_AND,
    BITWISE_XOR,
    BITWISE_LSHIFT,
    BITWISE_RSHIFT,
    POW,
    MOD,
    ADD,
    SUB,
    MUL,
    DIV,
    EQ,
    NE,
    GE,
    LE,
    GT,
    LT,
    LOGICAL_NEGATION,
    BITWISE_NEGATION,
    ARITHMETIC_NEGATION,
    INT_CAST,
    WORLD_CAST,
  };

  // Child nodes passed to constructors or add transfer ownership, and are
  // destroyed when the parent is destroyed. These would take explicit
  // y::unique<Node> parameters but for sake of brevity in the parser.
  Node(node_type type);
  Node(node_type type, Node* a);
  Node(node_type type, Node* a, Node* b);
  Node(node_type type, Node* a, Node* b, Node* c);

  Node(node_type type, y::int32 value);
  Node(node_type type, y::world value);
  Node(node_type type, y::string value);

  void add(Node* node);

  // Information about the location of this Node in the source text, for
  // diagnostic purposes.
  y::size line;
  y::string text;

  // Type and children.
  node_type type;
  y::vector<y::unique<Node>> children;

  // Literal values.
  y::int32 int_value;
  y::world world_value;
  y::string string_value;

  // Get the human-readable text of an operator.
  static y::string op_string(node_type t);

  // If parsing aborts half-way due to a syntax error, Nodes allocated by the
  // parser will leak. To avoid this, we keep a set of Nodes which have been
  // allocated but not inserted as children so they can be freed later.
  static y::set<Node*> orphans;

};

#endif
