#include "type.h"

Type::Type(type_base base, y::size count)
  : _base(base)
  , _count(count)
  , _const(false)
{
  if (count == 0 ||
      (count != 1 && base != INT && base != WORLD)) {
    _base = ERROR;
    _count = 1;
  }
}

void Type::set_const(bool is_const)
{
  _const = is_const;
}

Type::type_base Type::base() const
{
  return _base;
}

y::size Type::count() const
{
  return _count;
}

bool Type::is_const() const
{
  return _const;
}

y::string Type::string() const
{
  y::string s =
      _base == VOID ? "void" :
      _base == INT ? "int" :
      _base == WORLD ? "world" : "error";

  if (_count > 1) {
    s += y::to_string(_count);
  }
  return "`" + s + "`";
}

bool Type::is_error() const
{
  return _base == ERROR;
}

bool Type::not_void() const
{
  return _base != VOID;
}

bool Type::primitive() const
{
  return is_error() ||
      (_count == 1 && (is_int() || is_world()));
}

bool Type::is_vector() const
{
  return is_error() ||
      (_count > 1 && (is_int() || is_world()));
}

bool Type::is_int() const
{
  return is_error() || _base == INT;
}

bool Type::is_world() const
{
  return is_error() || _base == WORLD;
}

bool Type::count_binary_match(const Type& t) const
{
  return is_error() || t.is_error() ||
      count() == t.count() || count() == 1 || t.count() == 1;
}

bool Type::is(const Type& t) const
{
  return *this == t || is_error() || t.is_error();
}

Type Type::unify(const Type& t) const
{
  return *this != t ? ERROR : *this;
}

bool Type::operator==(const Type& t) const
{
  return _base == t._base && _count == t._count;
}

bool Type::operator!=(const Type& t) const
{
  return !(*this == t);
}


