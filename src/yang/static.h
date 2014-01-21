#ifndef YANG__STATIC_H
#define YANG__STATIC_H

#include "internal_type.h"
#include "table.h"
#include "type.h"
#include "walker.h"

namespace yang {
namespace internal {

class StaticChecker : public ConstAstWalker<Type> {
public:

  typedef y::map<y::string, yang::Type> symbol_frame;
  StaticChecker(const symbol_frame& context_functions,
                symbol_frame& functions_output, symbol_frame& globals_output);
  ~StaticChecker();

  // True if any errors were detected during the checking. Otherwise, assuming
  // there are no bugs in the compiler, we can generate the IR without worrying
  // about malformed AST.
  bool errors() const;

protected:

  void preorder(const Node& node) override;
  void infix(const Node& node, const result_list& results) override;
  Type visit(const Node& node, const result_list& results) override;

private:

  void enter_function(const Type& return_type);
  const Type& current_return_type() const;
  bool inside_function() const;

  bool use_function_immediate_assign_hack(const Node& node) const;
  void add_symbol_checking_collision(
      const Node& node, const y::string& name, const Type& type);
  void add_symbol_checking_collision(
      const Node& node, const y::string& name, y::size index, const Type& type);
  void error(const Node& node, const y::string& message);

  struct function {
    Type return_type;
    y::string name;
  };

  bool _errors;
  y::string _current_function;
  y::string _immediate_left_assign;

  enum metadata {
    EXPORT_GLOBAL,
    LOOP_BODY,
    RETURN_TYPE,
  };
  friend std::hash<metadata>;

  SymbolTable<metadata, Type> _metadata;
  SymbolTable<y::string, Type> _symbol_table;

  const symbol_frame& _context_functions;
  symbol_frame& _functions_output;
  symbol_frame& _globals_output;

};

// End namespace yang::internal.
}
}

#endif
