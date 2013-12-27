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

  llvm::Value* visit(const Node& node, const result_list& results) override;

private:

  llvm::IRBuilder<> _builder;

};

#endif
