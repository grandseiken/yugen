#ifndef YANG__STATIC_H
#define YANG__STATIC_H

#include "table.h"
#include "type.h"
#include "walker.h"

class StaticChecker : public ConstAstWalker<Type> {
public:

  StaticChecker();

  // True if any errors were detected during the checking. Otherwise, assuming
  // there are no bugs in the compiler, we can generate the IR without worrying
  // about malformed AST.
  bool errors() const;

  // After checking, returns a mapping of global variable names to types.
  const y::map<y::string, Type>& global_variable_map() const;

protected:

  void preorder(const Node& node) override;
  void infix(const Node& node, const result_list& results) override;
  Type visit(const Node& node, const result_list& results) override;

private:

  void error(const Node& node, const y::string& message);

  struct function {
    Type return_type;
    y::string name;
  };

  bool _errors;
  function _current_function;
  SymbolTable<Type> _symbol_table;
  y::map<y::string, Type> _global_variable_map;

};

#endif
