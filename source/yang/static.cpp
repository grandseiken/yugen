#include "static.h"
#include "pipeline.h"
#include "../common/algorithm.h"
#include "../log.h"

StaticChecker::StaticChecker()
  : _errors(false)
  , _current_function{Type::VOID, ""}
  , _symbol_table(Type::VOID)
{
}

bool StaticChecker::errors() const
{
  return _errors;
}

const y::map<y::string, Type>& StaticChecker::global_variable_map() const
{
  return _global_variable_map;
}

void StaticChecker::preorder(const Node& node)
{
  switch (node.type) {
    case Node::GLOBAL:
      _symbol_table.push();
      _symbol_table.add("%GLOBAL%", Type::VOID);
      break;
    case Node::FUNCTION:
    {
      // Insert the function into the first frame before adding the next frame.
      if (_symbol_table.has(node.string_value)) {
        if (!_symbol_table[node.string_value].is_error()) {
          error(node, "global `" + node.string_value + "` redefined");
        }
        _symbol_table.remove(node.string_value);
      }
      Type t(Type::FUNCTION, Type(Type::INT));
      t.set_const(true);
      _symbol_table.add(node.string_value, t);

      _current_function.return_type = Type::INT;
      _current_function.name = node.string_value;
    }
    case Node::BLOCK:
    case Node::IF_STMT:
      _symbol_table.push();
      break;
    case Node::DO_WHILE_STMT:
    case Node::FOR_STMT:
      _symbol_table.push();
      // Insert a marker into the symbol table that break and continue
      // statements can check for.
      _symbol_table.add("%LOOP_BODY%", Type::VOID);

    default: {}
  }
}

void StaticChecker::infix(const Node& node, const result_list& results)
{
  (void)node;
  (void)results;
}

Type StaticChecker::visit(const Node& node, const result_list& results)
{
  y::string s = "`" + Node::op_string(node.type) + "`";
  y::vector<y::string> rs;
  for (Type t : results) {
    rs.push_back(t.string());
  }

  // To make the error messages useful, the general idea here is to fall back to
  // the ERROR type only when the operands (up to errors) do not uniquely
  // determine some result type.
  // For example, the erroneous (1, 1) + (1, 1, 1) results in an ERROR type,
  // since there's no way to decide if an int2 or int3 was intended. However,
  // the erroneous 1 == 1. gives type INT, as the result would be INT whether or
  // not the operand type was intended to be int or world.
  switch (node.type) {
    case Node::PROGRAM:
      return Type::VOID;
    case Node::GLOBAL:
      _symbol_table.pop();
      return Type::VOID;
    case Node::FUNCTION:
      if (!results[0].not_void()) {
        error(node, "not all code paths return a value");
      }
      _symbol_table.pop();
      _current_function.return_type = Type::VOID;
      _current_function.name = "";
      return Type::VOID;

    case Node::BLOCK:
    {
      _symbol_table.pop();
      // The code for RETURN_STMT checks return values against the function's
      // return type. We don't really care what types might be here, just any
      // non-void as a marker for ensuring all code paths return a value.
      for (const Type& t : results) {
        if (t.not_void()) {
          return t;
        }
      }
      return Type::VOID;
    }

    case Node::EMPTY_STMT:
    case Node::EXPR_STMT:
      return Type::VOID;
    case Node::RETURN_STMT:
      if (_symbol_table.has("%GLOBAL%")) {
        error(node, "return statement inside `global`");
      }
      else if (!results[0].is(_current_function.return_type)) {
        error(node, "returning " + rs[0] + " from " +
                    _current_function.return_type.string() + " function");
      }
      return results[0];
    case Node::IF_STMT:
    {
      _symbol_table.pop();
      if (!results[0].is(Type::INT)) {
        error(node, "branching on " + rs[0]);
      }
      // An IF_STMT definitely returns a value only if both branches definitely
      // return a value.
      Type left = results[1];
      Type right = results.size() > 2 ? results[2] : Type::VOID;
      return left.not_void() && right.not_void() ? left : Type::VOID;
    }
    case Node::DO_WHILE_STMT:
    case Node::FOR_STMT:
      _symbol_table.pop();
      if (!results[1].is(Type::INT)) {
        error(node, "branching on " + rs[1]);
      }
      return Type::VOID;
    case Node::BREAK_STMT:
      if (!_symbol_table.has("%LOOP_BODY%")) {
        error(node, "`break` outside of loop body");
      }
      return Type::VOID;
    case Node::CONTINUE_STMT:
      if (!_symbol_table.has("%LOOP_BODY%")) {
        error(node, "`continue` outside of loop body");
      }
      return Type::VOID;

    case Node::IDENTIFIER:
      if (!_symbol_table.has(node.string_value)) {
        error(node, "undeclared identifier `" + node.string_value + "`");
        _symbol_table.add(node.string_value, Type::ERROR);
      }
      return _symbol_table[node.string_value];
    case Node::INT_LITERAL:
      return Type::INT;
    case Node::WORLD_LITERAL:
      return Type::WORLD;

    case Node::TERNARY:
      // TODO: ternary vectorisation.
      if (!results[1].is(results[2])) {
        error(node, s + " applied to " + rs[1] + " and " + rs[2]);
      }
      if (!results[0].is(Type::INT)) {
        error(node, s + " branching on " + rs[0]);
      }
      return results[1].unify(results[2]);
    case Node::CALL:
      if (!results[0].function()) {
        error(node, s + " applied to " + rs[0]);
        return Type::ERROR;
      }
      if (!results[0].element_size(results.size())) {
        error(node, rs[0] + " called with " +
                    y::to_string(results.size() - 1) + " argument(s)");
      }
      else {
        for (y::size i = 1; i < results.size(); ++i) {
          if (!results[0].element_is(i, results[i])) {
            error(node, rs[0] + " called with " + rs[i] +
                        " in position " + y::to_string(i));
          }
        }
      }
      return results[0].elements()[0];

    case Node::LOGICAL_OR:
    case Node::LOGICAL_AND:
    case Node::BITWISE_OR:
    case Node::BITWISE_AND:
    case Node::BITWISE_XOR:
    case Node::BITWISE_LSHIFT:
    case Node::BITWISE_RSHIFT:
      // Takes two integers and produces an integer, with vectorisation.
      if (!results[0].count_binary_match(results[1])) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
        return Type::ERROR;
      }
      else if (!results[0].is_int() || !results[1].is_int()) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
      }
      return Type(Type::INT, y::max(results[0].count(), results[1].count()));

    case Node::POW:
    case Node::MOD:
    case Node::ADD:
    case Node::SUB:
    case Node::MUL:
    case Node::DIV:
      // Takes two integers or worlds and produces a value of the same type,
      // with vectorisation.
      if (!results[0].count_binary_match(results[1]) ||
          (!(results[0].is_int() && results[1].is_int()) &&
           !(results[0].is_world() && results[1].is_world()))) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
        return Type::ERROR;
      }
      return Type(results[0].base(),
                  y::max(results[0].count(), results[1].count()));

    case Node::EQ:
    case Node::NE:
    case Node::GE:
    case Node::LE:
    case Node::GT:
    case Node::LT:
      // Takes two integers or worlds and produces an integer, with
      // vectorisation.
      if (!results[0].count_binary_match(results[1])) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
        return Type::ERROR;
      }
      else if (!(results[0].is_int() && results[1].is_int()) &&
               !(results[0].is_world() && results[1].is_world())) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
      }
      return Type(Type::INT, y::max(results[0].count(), results[1].count()));

    case Node::FOLD_LOGICAL_OR:
    case Node::FOLD_LOGICAL_AND:
    case Node::FOLD_BITWISE_OR:
    case Node::FOLD_BITWISE_AND:
    case Node::FOLD_BITWISE_XOR:
    case Node::FOLD_BITWISE_LSHIFT:
    case Node::FOLD_BITWISE_RSHIFT:
      if (!results[0].is_vector() || !results[0].is_int()) {
        error(node, s + " applied to " + rs[0]);
      }
      return Type::INT;

    case Node::FOLD_POW:
    case Node::FOLD_MOD:
    case Node::FOLD_ADD:
    case Node::FOLD_SUB:
    case Node::FOLD_MUL:
    case Node::FOLD_DIV:
      if (!results[0].is_vector() ||
          !(results[0].is_int() || results[0].is_world())) {
        error(node, s + " applied to " + rs[0]);
        return Type::ERROR;
      }
      return results[0].base();

    case Node::FOLD_EQ:
    case Node::FOLD_NE:
    case Node::FOLD_GE:
    case Node::FOLD_LE:
    case Node::FOLD_GT:
    case Node::FOLD_LT:
      if (!results[0].is_vector() ||
          !(results[0].is_int() || results[0].is_world())) {
        error(node, s + " applied to " + rs[0]);
      }
      return Type::INT;

    case Node::LOGICAL_NEGATION:
    case Node::BITWISE_NEGATION:
      if (!results[0].is_int()) {
        error(node, s + " applied to " + rs[0]);
      }
      return Type(Type::INT, results[0].count());

    case Node::ARITHMETIC_NEGATION:
      if (!(results[0].is_int() || results[0].is_world())) {
        error(node, s + " applied to " + rs[0]);
        return Type::ERROR;
      }
      return results[0];

    case Node::ASSIGN:
    {
      if (!_symbol_table.has(node.string_value)) {
        error(node, "undeclared identifier `" + node.string_value + "`");
        return Type::ERROR;
      }
      Type& t = _symbol_table[node.string_value];
      if (t.is_const()) {
        error(node, "assignment to `" + node.string_value +
                    "` of type " + t.string());
      }
      if (!t.is(results[0])) {
        error(node, rs[0] + " assigned to `" + node.string_value +
                    "` of type " + t.string());
      }
      t = results[0];
      return results[0];
    }

    case Node::ASSIGN_VAR:
    case Node::ASSIGN_CONST:
      // Within global blocks, use the top-level symbol table frame.
      if (_symbol_table.has("%GLOBAL%")) {
        if (_symbol_table.has(node.string_value, 0)) {
          if (!_symbol_table.get(node.string_value, 0).is_error()) {
            error(node, "global `" + node.string_value + "` redefined");
          }
          _symbol_table.remove(node.string_value, 0);
        }
        _symbol_table.add(node.string_value, 0, results[0]);
        _symbol_table.get(node.string_value, 0).set_const(
            node.type == Node::ASSIGN_CONST);
        // Store global in the global map for future use.
        _global_variable_map.emplace(
            node.string_value, _symbol_table.get(node.string_value, 0));
        return results[0];
      }

      if (_symbol_table.has_top(node.string_value)) {
        if (!_symbol_table[node.string_value].is_error()) {
          error(node, "`" + node.string_value + "` redefined");
        }
        _symbol_table.remove(node.string_value);
      }
      _symbol_table.add(node.string_value, results[0]);
      _symbol_table[node.string_value].set_const(
          node.type == Node::ASSIGN_CONST);
      return results[0];

    case Node::INT_CAST:
      if (!results[0].is_world()) {
        error(node, s + " applied to " + rs[0]);
      }
      return Type(Type::INT, results[0].count());

    case Node::WORLD_CAST:
      if (!results[0].is_int()) {
        error(node, s + " applied to " + rs[0]);
      }
      return Type(Type::WORLD, results[0].count());

    case Node::VECTOR_CONSTRUCT:
    {
      Type t = results[0];
      y::string ts;
      bool unify_error = false;
      for (y::size i = 0; i < results.size(); ++i) {
        if (!results[i].primitive()) {
          error(node, s + " element with non-primitive type " + rs[i]);
          t = Type::ERROR;
        }
        if (i) {
          bool error = t.is_error();
          t = t.unify(results[i]);
          if (!error && t.is_error()) {
            unify_error = true;
          }
          ts += ", ";
        }
        ts += rs[i];
      }
      if (unify_error) {
        error(node, s + " applied to different types " + ts);
      }
      return Type(t.base(), results.size());
    }
    case Node::VECTOR_INDEX:
      if (!results[0].is_vector() || !results[1].is(Type::INT)) {
        error(node, s + " applied to " + rs[0] + " and " + rs[1]);
        return results[0].is_vector() ? results[0].base() : Type::ERROR;
      }
      return results[0].base();

    default:
      error(node, "unimplemented construct");
      return Type::ERROR;
  }
}

void StaticChecker::error(const Node& node, const y::string& message)
{
  _errors = true;
  y::string m = message;
  if (_current_function.name.length()) {
    m = "in function `" + _current_function.name + "`: " + m;
  }
  log_err(ParseGlobals::error(node.line, node.text, m));
}
