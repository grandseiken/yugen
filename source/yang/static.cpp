#include "static.h"
#include "../log.h"

Type::Type(type_base base, y::size count)
  : _base(count ? base : ERROR)
  , _count(count)
{
}

Type::type_base Type::base() const
{
  return _base;
}

y::size Type::count() const
{
  return _count;
}

y::string Type::string() const
{
  y::string s =
      _base == INT ? "int" :
      _base == WORLD ? "world" : "error";

  if (_count > 1) {
    s += y::to_string(_count);
  }
  return "`" + s + "`";
}

Type Type::unify(const Type& t) const
{
  return *this != t ? ERROR : *this;
}

bool Type::is(const Type& t) const
{
  return *this == t || *this == ERROR || t == ERROR;
}

bool Type::is(const Type& t, const Type& u) const
{
  return (*this == t && t == u) ||
      *this == ERROR || t == ERROR || u == ERROR;
}

bool Type::operator==(const Type& t) const
{
  return _base == t._base && _count == t._count;
}

bool Type::operator!=(const Type& t) const
{
  return !(*this == t);
}

StaticChecker::StaticChecker()
  : _errors(false)
{
}

bool StaticChecker::errors() const
{
  return _errors;
}

Type StaticChecker::visit(const Node& node, const result_list& results)
{
  y::string s = "`" + Node::op_string(node.type) + "`";
  y::vector<y::string> rs;
  for (Type t : results) {
    rs.push_back(t.string());
  }

  switch (node.type) {
    case Node::IDENTIFIER:
      error(node, "identifiers unsupported");
      return Type::ERROR;
    case Node::INT_LITERAL:
      return Type::INT;
    case Node::WORLD_LITERAL:
      return Type::WORLD;

    case Node::TERNARY:
      if (!results[1].is(results[2])) {
        error(node, s + " applied to " + rs[1] + " and " + rs[2]);
      }
      if (!results[0].is(Type::INT)) {
        error(node, s + " branching on " + rs[0]);
        return Type::ERROR;
      }
      return results[0].unify(results[1]);

    case Node::LOGICAL_OR:
    case Node::LOGICAL_AND:
    case Node::BITWISE_OR:
    case Node::BITWISE_AND:
    case Node::BITWISE_XOR:
    case Node::BITWISE_LSHIFT:
    case Node::BITWISE_RSHIFT:
      if (!results[0].is(results[1], Type::INT)) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
      }
      return Type::INT;

    case Node::POW:
    case Node::MOD:
    case Node::ADD:
    case Node::SUB:
    case Node::MUL:
    case Node::DIV:
      if (!results[0].is(results[1], Type::INT) &&
          !results[0].is(results[1], Type::WORLD)) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
        return Type::ERROR;
      }
      return results[0].unify(results[1]);

    case Node::EQ:
    case Node::NE:
    case Node::GE:
    case Node::LE:
    case Node::GT:
    case Node::LT:
      if (!results[0].is(results[1], Type::INT) &&
          !results[0].is(results[1], Type::WORLD)) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
      }
      return Type::INT;

    case Node::LOGICAL_NEGATION:
    case Node::BITWISE_NEGATION:
      if (!results[0].is(Type::INT)) {
        error(node, s + " applied to " + rs[0]);
      }
      return Type::INT;

    case Node::ARITHMETIC_NEGATION:
      if (!results[0].is(Type::INT) && results[0].is(Type::WORLD)) {
        error(node, s + " applied to " + rs[0]);
        return Type::ERROR;
      }
      return results[0];

    case Node::INT_CAST:
      if (!results[0].is(Type::WORLD)) {
        error(node, s + " applied to " + rs[0]);
      }
      return Type::INT;

    case Node::WORLD_CAST:
      if (!results[0].is(Type::INT)) {
        error(node, s + " applied to " + rs[0]);
      }
      return Type::WORLD;
  }

  // Default.
  error(node, "unimplemented construct");
  return Type::ERROR;
}

void StaticChecker::error(const Node& node, const y::string& message)
{
  _errors = true;
  log_err("Error at line ", node.line,
          ", near `", node.text, "`:\n\t", message);
}
