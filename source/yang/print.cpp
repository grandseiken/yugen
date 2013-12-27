#include "print.h"

y::string AstPrinter::visit(const Node& node, const result_list& results)
{
  y::string s = Node::op_string(node.type);

  switch (node.type) {
    case Node::IDENTIFIER:
      return node.string_value;
    case Node::INT_LITERAL:
      return y::to_string(node.int_value);
    case Node::WORLD_LITERAL:
      return y::to_string(node.world_value);

    case Node::TERNARY:
      return "(" + results[0] + " ? " + results[1] + " : " + results[2] + ")";

    case Node::LOGICAL_OR:
    case Node::LOGICAL_AND:
    case Node::BITWISE_OR:
    case Node::BITWISE_AND:
    case Node::BITWISE_XOR:
    case Node::BITWISE_LSHIFT:
    case Node::BITWISE_RSHIFT:
    case Node::POW:
    case Node::MOD:
    case Node::ADD:
    case Node::SUB:
    case Node::MUL:
    case Node::DIV:
    case Node::EQ:
    case Node::NE:
    case Node::GE:
    case Node::LE:
    case Node::GT:
    case Node::LT:
      return "(" + results[0] + " " + s + " " + results[1] + ")";

    case Node::LOGICAL_NEGATION:
    case Node::BITWISE_NEGATION:
    case Node::ARITHMETIC_NEGATION:
      return s + results[0];

    case Node::INT_CAST:
      return "[" + results[0] + "]";
    case Node::WORLD_CAST:
      return results[0] + ".";

    case Node::VECTOR_CONSTRUCT:
    {
      y::string output = "";
      for (const y::string& s : results) {
        if (output.length()) {
          output += ", ";
        }
        output += s;
      }
      return "(" + output + ")";
    }
  }
}
