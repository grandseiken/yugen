#ifndef YANG__IRGEN_H
#define YANG__IRGEN_H

#include "table.h"
#include "type.h"
#include "walker.h"
#include "../common/function.h"
#include <llvm/IR/IRBuilder.h>

namespace llvm {
  class Constant;
  class Function;
  class Module;
  class Type;
  class Value;
}

struct IrGeneratorUnion {
  IrGeneratorUnion(llvm::Type* type);
  IrGeneratorUnion(llvm::Value* value);

  operator llvm::Type*() const;
  operator llvm::Value*() const;

  llvm::Type* type;
  llvm::Value* value;
};

class IrGenerator : public ConstAstWalker<IrGeneratorUnion> {
public:

  IrGenerator(llvm::Module& module,
              const SymbolTable<Type>::scope& globals);
  ~IrGenerator();

  // Emit functions for allocating, freeing, reading and writing to instances
  // of the global structure. This should be called after the tree has been
  // walked!
  void emit_global_functions();

protected:

  void preorder(const Node& node) override;
  void infix(const Node& node, const result_list& results) override;
  IrGeneratorUnion visit(const Node& node, const result_list& results) override;

private:

  llvm::Type* void_type() const;
  llvm::Type* int_type() const;
  llvm::Type* world_type() const;
  llvm::Type* vector_type(llvm::Type* type, y::size n) const;

  llvm::Constant* constant_int(y::int32 value) const;
  llvm::Constant* constant_world(y::world value) const;
  llvm::Constant* constant_vector(
      const y::vector<llvm::Constant*>& values) const;
  llvm::Constant* constant_vector(llvm::Constant* value, y::size n) const;

  llvm::Value* i2b(llvm::Value* v);
  llvm::Value* b2i(llvm::Value* v);
  llvm::Value* global_ptr(llvm::Value* ptr, y::size index);
  llvm::Value* global_ptr(const y::string& name);

  // Euclidean mod and div implementations.
  llvm::Value* mod(llvm::Value* v, llvm::Value* u);
  llvm::Value* div(llvm::Value* v, llvm::Value* u);

  llvm::Value* binary(
      llvm::Value* left, llvm::Value* right,
      y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op);
  llvm::Value* fold(
      llvm::Value* value,
      y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op,
      bool to_bool = false, bool with_ands = false);

  // Get the equivalent LLVM type for a Yang type.
  llvm::Type* get_llvm_type(const Type& t) const;

  // List of static initialisation functions.
  y::vector<llvm::Function*> _global_inits;
  // Map from global name to index in the global structure.
  y::map<y::string, y::size> _global_numbering;
  // Type of the global structure.
  llvm::Type* _global_data;

  llvm::Module& _module;
  llvm::IRBuilder<> _builder;
  SymbolTable<llvm::Value*> _symbol_table;
  y::string _immediate_left_assign;

};

#endif
