#include "type.h"
#include "../log.h"

#include <boost/functional/hash.hpp>

namespace yang {

y::string Type::string() const
{
  y::string s;
  if (_base == USER_TYPE) {
    s = _user_type_name + "*";
  }
  else if (_base == FUNCTION) {
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
  return 1 + index >= _elements.size() ? void_type : _elements[1 + index];
}

bool Type::is_user_type() const
{
  return _base == USER_TYPE;
}

const y::string& Type::get_user_type_name() const
{
  return _user_type_name;
}

bool Type::operator==(const Type& t) const
{
  if (_elements.size() != t._elements.size()) {
    return false;
  }
  for (y::size i = 0; i < _elements.size(); ++i) {
    if (_elements[i] != t._elements[i]) {
      return false;
    }
  }
  return _base == t._base && _count == t._count;
}

bool Type::operator!=(const Type& t) const
{
  return !operator==(t);
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

namespace std {
  y::size hash<yang::Type>::operator()(const yang::Type& type) const
  {
    y::size seed = 0;
    boost::hash_combine(seed, type._base);
    boost::hash_combine(seed, type._count);
    for (const auto& t : type._elements) {
      boost::hash_combine(seed, operator()(t));
    }
    return seed;
  }
}
