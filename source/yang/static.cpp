#include "static.h"
#include "../log.h"

namespace {

  Type unify(Type a, Type b)
  {
    return a != b ? TYPE_ERROR : a;
  }

  bool type_is(Type a, Type b)
  {
    return a == b || a == TYPE_ERROR || b == TYPE_ERROR;
  }

  bool type_is(Type a, Type b, Type c)
  {
    return (a == b && b == c) ||
        a == TYPE_ERROR || b == TYPE_ERROR || c == TYPE_ERROR;
  }

  y::string type_string(Type t)
  {
    y::string s =
        t == TYPE_INT ? "int" :
        t == TYPE_WORLD ? "world" : "error";
    return "`" + s + "`";
  }

  y::string op_string(Node::node_type t)
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

}

StaticChecker::StaticChecker()
  : _errors(false)
{
}

Type StaticChecker::visit(const Node& node, const result_list& results)
{
  y::string s = op_string(node.type);
  y::vector<y::string> rs;
  for (Type t : results) {
    rs.push_back(type_string(t));
  }

  switch (node.type) {
    case Node::IDENTIFIER:
      error(node, "identifiers unsupported");
      return TYPE_ERROR;
    case Node::INT_LITERAL:
      return TYPE_INT;
    case Node::WORLD_LITERAL:
      return TYPE_WORLD;

    case Node::TERNARY:
      if (!type_is(results[1], results[2])) {
        error(node, s + " applied to " + rs[1] + " and " + rs[2]);
      }
      if (!type_is(results[0], TYPE_INT)) {
        error(node, s + " branching on " + rs[0]);
        return TYPE_ERROR;
      }
      return unify(results[0], results[1]);

    case Node::LOGICAL_OR:
    case Node::LOGICAL_AND:
    case Node::BITWISE_OR:
    case Node::BITWISE_AND:
    case Node::BITWISE_XOR:
    case Node::BITWISE_LSHIFT:
    case Node::BITWISE_RSHIFT:
      if (!type_is(results[0], results[1], TYPE_INT)) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
      }
      return TYPE_INT;

    case Node::POW:
    case Node::MOD:
    case Node::ADD:
    case Node::SUB:
    case Node::MUL:
    case Node::DIV:
      if (!type_is(results[0], results[1], TYPE_INT) &&
          !type_is(results[0], results[1], TYPE_WORLD)) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
        return TYPE_ERROR;
      }
      return unify(results[0], results[1]);

    case Node::EQ:
    case Node::NE:
    case Node::GE:
    case Node::LE:
    case Node::GT:
    case Node::LT:
      if (!type_is(results[0], results[1], TYPE_INT) &&
          !type_is(results[0], results[1], TYPE_WORLD)) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
      }
      return TYPE_INT;

    case Node::LOGICAL_NEGATION:
    case Node::BITWISE_NEGATION:
      if (!type_is(results[0], TYPE_INT)) {
        error(node, s + " applied to " + rs[0]);
      }
      return TYPE_INT;

    case Node::ARITHMETIC_NEGATION:
      if (!type_is(results[0], TYPE_INT) &&
          !type_is(results[0], TYPE_WORLD)) {
        error(node, s + " applied to " + rs[0]);
        return TYPE_ERROR;
      }
      return results[0];

    case Node::INT_CAST:
      if (!type_is(results[0], TYPE_WORLD)) {
        error(node, s + " applied to " + rs[0]);
      }
      return TYPE_INT;

    case Node::WORLD_CAST:
      if (!type_is(results[0], TYPE_INT)) {
        error(node, s + " applied to " + rs[0]);
      }
      return TYPE_WORLD;
  }

  return TYPE_ERROR;
}

bool StaticChecker::errors() const
{
  return _errors;
}

void StaticChecker::error(const Node& node, const y::string& message)
{
  _errors = true;
  log_err("error at line ", node.line,
          ", near `", node.text, "`:\n\t", message);
}
