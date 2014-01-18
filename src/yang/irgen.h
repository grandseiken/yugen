#ifndef YANG__IRGEN_H
#define YANG__IRGEN_H

#include "table.h"
#include "type.h"
#include "walker.h"
#include "../common/function.h"
#include <llvm/IR/IRBuilder.h>

namespace llvm {
  class Constant;
  class ExecutionEngine;
  class Function;
  class Module;
  class Type;
  class Value;
}

namespace yang {
namespace internal {

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

  typedef y::map<y::string, yang::Type> symbol_frame;
  IrGenerator(llvm::Module& module, llvm::ExecutionEngine& engine,
              symbol_frame& globals);
  ~IrGenerator();

  // Emit functions for allocating, freeing, reading and writing to instances
  // of the global structure. This should be called after the tree has been
  // walked!
  void emit_global_functions();

  typedef y::map<y::string, llvm::Function*> trampoline_map;
  const trampoline_map& get_trampoline_map() const;

protected:

  void preorder(const Node& node) override;
  void infix(const Node& node, const result_list& results) override;
  IrGeneratorUnion visit(const Node& node, const result_list& results) override;

private:

  // Get an LLVM function pointer to a native function.
  typedef void (*void_fp)();
  llvm::Function* get_native_function(
      const y::string& name, void_fp native_fp, llvm::FunctionType* type) const;

  // Tools for functions and calling conventions.
  void create_function(
      const Node& node, llvm::FunctionType* function_type);
  // Generate trampoline function for converting C calling convention to LLVM
  // calling convention.
  llvm::Function* create_trampoline_function(llvm::FunctionType* function_type);

  // General helper functions.
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
  llvm::Value* i2w(llvm::Value* v);
  llvm::Value* w2i(llvm::Value* v);

  llvm::Value* global_ptr(llvm::Value* ptr, y::size index);
  llvm::Value* global_ptr(const y::string& name);

  // Power implementation.
  llvm::Value* pow(llvm::Value* v, llvm::Value* u);
  // Euclidean mod and div implementations.
  llvm::Value* mod(llvm::Value* v, llvm::Value* u);
  llvm::Value* div(llvm::Value* v, llvm::Value* u);

  llvm::Value* binary(
      llvm::Value* left, llvm::Value* right,
      y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op);
  llvm::Value* fold(
      llvm::Value* value,
      y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op,
      bool to_bool = false, bool with_ands = false, bool right_assoc = false);

  // Get the equivalent LLVM type for a Yang type.
  llvm::Type* get_llvm_type(const yang::Type& t) const;

  // Metadata symbols.
  enum metadata {
    GLOBAL_DATA_PTR,
    GLOBAL_INIT_FUNCTION,
    FUNCTION,

    IF_THEN_BLOCK,
    IF_ELSE_BLOCK,

    LOOP_COND_BLOCK,
    LOOP_BODY_BLOCK,
    LOOP_AFTER_BLOCK,

    LOOP_BREAK_LABEL,
    LOOP_CONTINUE_LABEL,

    LOGICAL_OP_SOURCE_BLOCK,
    LOGICAL_OP_RHS_BLOCK,

    MERGE_BLOCK,
  };

  // Create block and insert in the metadata table.
  llvm::BasicBlock* create_block(metadata meta, const y::string& name);

  // List of static initialisation functions.
  y::vector<llvm::Function*> _global_inits;
  // Map from global name to index in the global structure.
  y::map<y::string, y::size> _global_numbering;
  // Type of the global structure.
  llvm::Type* _global_data;

  // Generated trampolines.
  y::map<llvm::FunctionType*, llvm::Function*> _trampoline_functions;
  // Map from function name to corresponding trampoline.
  trampoline_map _trampoline_map;

  llvm::Module& _module;
  llvm::ExecutionEngine& _engine;
  llvm::IRBuilder<> _builder;

  // We keep a second symbol table for special metadata entries that don't
  // correspond to actual source code symbols; this way we can add scopes
  // that automatically pop metadata without interfering with scope lookup.
  friend std::hash<metadata>;
  SymbolTable<y::string, llvm::Value*> _symbol_table;
  SymbolTable<metadata, llvm::Value*> _metadata;
  // Metadata that isn't an llvm::Value.
  y::string _immediate_left_assign;

};

// End namespace yang::internal.
}
}

#endif
