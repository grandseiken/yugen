#ifndef YANG__STATIC_H
#define YANG__STATIC_H

#include "table.h"
#include "walker.h"

// TYPE_ERROR is a special type assigned to expressions containing an error
// where the type cannot be determined. Further errors involving a value
// of this type are suppressed (to avoid cascading error messages).
class Type {
public:

  enum type_base {
    ERROR,
    VOID,
    INT,
    WORLD,
  };

  // A count greater than one constructs a vector type. This is allowed
  // only if the base is type INT or WORLD. Invalid construction will result
  // in an ERROR type.
  Type(type_base b, y::size count = 1);

  // Return components of the type.
  type_base base() const;
  y::size count() const;
  // Return a string representation of the type.
  y::string string() const;

  // All of the following functions also return true if this is type ERROR.
  // True if this is type ERROR.
  bool is_error() const;
  // True if this is type VOID.
  bool is_void() const;
  // True if this is type INT or WORLD with a count of 1.
  bool primitive() const;
  // True if this is type INT or WORLD with a count greater than 1.
  bool is_vector() const;
  // True if this is type INT (either primitive or vector).
  bool is_int() const;
  // True if this is type WORLD (either primitive or vector).
  bool is_world() const;

  // True if the vector element-counts of these types allow for interaction;
  // that is, either the element-counts are the same (and they can interact
  // point-wise), or either element-count is 1 (and the value can be implicitly
  // vectorised), or either type is type ERROR.
  bool count_binary_match(const Type& t) const;
  // True if the given type is identical, or either is type ERROR.
  bool is(const Type& t) const;
  // If the given type is identical, returns it. Otherwise, returns type ERROR.
  Type unify(const Type& t) const;

  // Raw equality comparisons (ignoring ERROR). Don't use for type-checking.
  bool operator==(const Type& t) const;
  bool operator!=(const Type& t) const;

private:

  type_base _base;
  y::size _count;

};

class StaticChecker : public ConstAstWalker<Type> {
public:

  StaticChecker();
  bool errors() const;

protected:

  void preorder(const Node& node) override;
  Type visit(const Node& node, const result_list& results) override;

private:

  void error(const Node& node, const y::string& message);

  SymbolTable<Type> _symbol_table;
  bool _errors;

};

#endif
