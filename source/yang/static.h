#ifndef YANG__STATIC_H
#define YANG__STATIC_H

#include "walker.h"
#include "table.h"

// TYPE_ERROR is a special type assigned to expressions containing an error
// where the type cannot be determined. Further errors involving a subtree
// of this type are suppressed (to avoid cascading error messages).
class Type {
public:

  enum type_base {
    ERROR,
    VOID,
    INT,
    WORLD,
  };

  Type(type_base b, y::size count = 1);

  type_base base() const;
  y::size count() const;
  y::string string() const;

  bool is_error() const;
  bool is_void() const;
  bool primitive() const;
  bool is_vector() const;
  bool is_int() const;
  bool is_world() const;

  bool count_binary_match(const Type& t) const;
  Type unify(const Type& t) const;
  bool is(const Type& t) const;

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
