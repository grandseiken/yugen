#ifndef YANG__PRINT_H
#define YANG__PRINT_H

#include "walker.h"

namespace llvm {
  class Value;
}

class IrGenerator : public ConstAstWalker<llvm::Value*> {
public:

  llvm::Value* visit(const Node& node, const result_list& results) override;

};

#endif
