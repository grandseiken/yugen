#ifndef YANG__IRGEN_H
#define YANG__IRGEN_H

#include "walker.h"
#include "../common/function.h"
#include <llvm/IR/IRBuilder.h>

namespace llvm {
  class Constant;
  class Module;
  class Value;
}

class IrGenerator : public ConstAstWalker<llvm::Value*> {
public:

  IrGenerator(llvm::Module& module);
  ~IrGenerator();

  void generate(const Node& node);

protected:

  llvm::Value* visit(const Node& node, const result_list& results) override;

private:

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
  llvm::Value* branch(llvm::Value* cond, llvm::Value* left, llvm::Value* right);

  llvm::Value* binary(
      llvm::Value* left, llvm::Value* right,
      y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op);
  llvm::Value* fold(
      llvm::Value* value,
      y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op,
      bool to_bool = false, bool with_ands = false);

  llvm::Module& _module;
  llvm::IRBuilder<> _builder;

};

#endif
