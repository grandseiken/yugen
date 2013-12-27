#ifndef YANG__IRGEN_H
#define YANG__IRGEN_H

#include "walker.h"
#include <llvm/IR/IRBuilder.h>

namespace llvm {
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

  llvm::Value* constant_int(y::int32 value) const;
  llvm::Value* constant_world(y::world value) const;
  llvm::Value* i2b(llvm::Value* v);
  llvm::Value* b2i(llvm::Value* v);

  llvm::Module& _module;
  llvm::IRBuilder<> _builder;

};

#endif
