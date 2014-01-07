#include "static.h"
#include "pipeline.h"
#include "../common/algorithm.h"
#include "../log.h"

StaticChecker::StaticChecker()
  : _errors(false)
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
      _current_function = ".global";
      _symbol_table.push();
      break;
    case Node::GLOBAL_ASSIGN:
      // Set current top-level function name.
      _current_function = node.string_value;
      // Fall-through to recursion handler.
    case Node::ASSIGN_VAR:
    case Node::ASSIGN_CONST:
      // Super big hack: in order to allow recursion, the function has to have
      // a name already in the scope of its body. This doesn't really make much
      // sense with function-expressions, but we can use a hack where, if a
      // function-expression appears immediately on the right-hand side of
      // assignment to a name, we make a note and store it in the symbol table
      // early.
      //
      // We don't allow different functions to be mutually-recursive, since that
      // necessarily requires a two-phase approach. However, it's not all that
      // important and can be achieved for any particular pair of functions by
      // nesting, anyway.
      // TODO: that doesn't actually work right now: it's considered an
      // "enclosing function reference". Need to be careful about this,
      // particularly with name-collision checking.
      if (node.children[0]->type == Node::FUNCTION) {
        _immediate_left_assign = node.string_value;
      }
      break;
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
      break;

    default: {}
  }
}

void StaticChecker::infix(const Node& node, const result_list& results)
{
  switch (node.type) {
    case Node::FUNCTION:
    {
      // Only append a suffix if this isn't a top-level function. Make use of
      // the recursive name hack, if it's there.
      if (inside_function()) {
        _current_function += _immediate_left_assign.length() ?
            "." + _immediate_left_assign : ".anon";
      }
      Type t = results[0];
      if (!t.function()) {
        error(node, "function defined with non-function type " + t.string());
        t = Type::ERROR;
      }

      // Functions need three symbol table frames: one to store the function's
      // immediate-left-hand name for recursive lookup, and enclosing-function-
      // -reference overrides; one for arguments; and one for the body.
      y::vector<y::string> locals;
      _symbol_table.get_symbols(locals, 1, _symbol_table.size());
      _symbol_table.push();
      // Do the recursive hack.
      if (_immediate_left_assign.length()) {
        _symbol_table.add(_immediate_left_assign, t);
        _immediate_left_assign = "";
      }
      // We currently don't implement closures at all, so we need to stick a
      // bunch of overrides into an intermediate stack frame to avoid the locals
      // from the enclosing scope being referenced, if any.
      // Starting at frame 1 finds all names except globals.
      for (const y::string& s : locals) {
        _symbol_table.add(s, Type::ENCLOSING_FUNCTION);
      }

      // Do the arguments.
      _symbol_table.push();
      if (!t.is_error()) {
        y::set<y::string> arg_names;
        y::size elem = 0;
        for (const auto& ptr : node.children[0]->children) {
          if (!elem) {
            ++elem;
            continue;
          }
          const y::string& name = ptr->string_value;
          if (!name.length()) {
            error(node, "function with unnamed argument");
            continue;
          }
          if (arg_names.find(name) != arg_names.end()) {
            error(node, "duplicate argument name `" + name + "`");
          }
          _symbol_table.add(name, t.elements(elem));
          arg_names.insert(name);
          ++elem;
        }
      }
      // Stores the return type of the current function and the fact we're
      // inside a function.
      enter_function(t.elements(0));
      _symbol_table.push();
      break;
    }

    default: {}
  }
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
    case Node::TYPE_VOID:
      return Type::VOID;
    case Node::TYPE_INT:
      return Type(Type::INT, node.int_value);
    case Node::TYPE_WORLD:
      return Type(Type::WORLD, node.int_value);
    case Node::TYPE_FUNCTION:
    {
      Type t(Type::FUNCTION, results[0]);
      for (y::size i = 1; i < results.size(); ++i) {
        if (!results[i].not_void()) {
          error(node, "function type with `void` argument type");
        }
        t.add_element(results[i]);
      }
      return t;
    }

    case Node::PROGRAM:
      return Type::VOID;
    case Node::GLOBAL:
      _symbol_table.pop();
      _current_function = "";
      return Type::VOID;
    case Node::GLOBAL_ASSIGN:
    {
      if (!results[0].function()) {
        error(node, "global assignment of type " + rs[0]);
      }
      if (_symbol_table.has_top(node.string_value)) {
        if (!_symbol_table[node.string_value].is_error()) {
          error(node, "global `" + node.string_value + "` redefined");
        }
        _symbol_table.remove(node.string_value);
      }
      // Top-level function variables are implicitly const.
      Type t = results[0];
      t.set_const(true);
      _symbol_table.add(node.string_value, t);
      _current_function = "";
      return Type::VOID;
    }
    case Node::FUNCTION:
    {
      if (current_return_type().not_void() && !results[1].not_void()) {
        error(node, "not all code paths return a value");
      }
      // Pop all the various symbol table frames a function uses.
      _symbol_table.pop();
      _symbol_table.pop();
      _symbol_table.pop();
      if (inside_function()) {
        _current_function =
            _current_function.substr(0, _current_function.find_last_of('.'));
      }
      return results[0];
    }

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
      // If we're not in a function, we must be in a global block.
      if (!inside_function()) {
        error(node, "return statement inside `global`");
      }
      else {
        const Type& current_return = current_return_type();
        if (!results[0].is(current_return)) {
          error(node, "returning " + rs[0] + " from " +
                      current_return.string() + " function");
        }
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
      else if (_symbol_table[node.string_value] == Type::ENCLOSING_FUNCTION) {
        error(node, "reference to `" + node.string_value +
                    "` in enclosing function");
        return Type::ERROR;
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
                        " in position " + y::to_string(i - 1));
          }
        }
      }
      return results[0].elements(0);

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
        _symbol_table.add(node.string_value, results[0]);
        return results[0];
      }
      Type& t = _symbol_table[node.string_value];
      if (t == Type::ENCLOSING_FUNCTION) {
        error(node, "reference to `" + node.string_value +
                    "` in enclosing function");
        return Type::ERROR;
      }
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
      if (!results[0].not_void()) {
        error(node, "assignment of type " + rs[0]);
      }

      // Within global blocks, use the top-level symbol table frame.
      if (!inside_function()) {
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

void StaticChecker::enter_function(const Type& return_type)
{
  _symbol_table.add("%CURRENT_RETURN_TYPE%", return_type);
}

const Type& StaticChecker::current_return_type() const
{
  return _symbol_table["%CURRENT_RETURN_TYPE%"];
}

bool StaticChecker::inside_function() const
{
  return _symbol_table.has("%CURRENT_RETURN_TYPE%");
}

void StaticChecker::error(const Node& node, const y::string& message)
{
  _errors = true;
  y::string m = message;
  if (_current_function.length()) {
    m = "in function `" + _current_function + "`: " + m;
  }
  log_err(ParseGlobals::error(node.line, node.text, m));
}
