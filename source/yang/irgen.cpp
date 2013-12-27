#include "irgen.h"

#include <llvm/IR/Module.h>

IrGenerator::IrGenerator(llvm::Module& module)
  : _builder(module.getContext())
{
}

IrGenerator::~IrGenerator()
{
}

llvm::Value* IrGenerator::visit(const Node& node, const result_list& results)
{
  return y::null;
}
