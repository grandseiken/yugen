#ifndef YANG__PRINT_H
#define YANG__PRINT_H

#include "walker.h"
#include "../common/string.h"

class AstPrinter : public ConstAstWalker<y::string> {
protected:

  void preorder(const Node& node) override;
  y::string visit(const Node& node, const result_list& results) override;

};

#endif
