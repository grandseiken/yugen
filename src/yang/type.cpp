#include "type.h"
#include "../log.h"

namespace yang {

y::string Type::string() const
{
  y::string s;
  if (_base == FUNCTION) {
    s += _elements[0].string() + "(";
    for (y::size i = 1; i < _elements.size(); ++i) {
      if (i > 1) {
        s += ", ";
      }
      s += _elements[i].string();
    }
    s += ")";
  }
  else {
    s += _base == VOID ? "void" :
         _base == INT ? "int" :
         _base == WORLD ? "world" : "error";

    if (_count > 1) {
      s += y::to_string(_count);
    }
  }
  return (_exported ? "export " : "") + s + (_const ? " const" : "");
}

bool Type::is_exported() const
{
  return _exported;
}

bool Type::is_const() const
{
  return _const;
}

bool Type::is_void() const
{
  return _base == VOID;
}

bool Type::is_int() const
{
  return _base == INT && _count == 1;
}

bool Type::is_world() const
{
  return _base == WORLD && _count == 1;
}

bool Type::is_int_vector() const
{
  return _base == INT && _count > 1;
}

bool Type::is_world_vector() const
{
  return _base == WORLD && _count > 1;
}

y::size Type::get_vector_size() const
{
  return _count;
}

bool Type::is_function() const
{
  return _base == FUNCTION;
}

y::size Type::get_function_num_args() const
{
  return _elements.empty() ? 0 : _elements.size() - 1;
}

const Type& Type::get_function_return_type() const
{
  return _elements.empty() ? void_type : _elements[0];
}

const Type& Type::get_function_arg_type(y::size index) const
{
  return index < _elements.size() ? void_type : _elements[1 + index];
}

Type::Type()
  : _exported(false)
  , _const(false)
  , _base(VOID)
  , _count(1)
{
}

Type Type::void_type;

// End namespace yang.
}
