#ifndef YANG__PRINT_H
#define YANG__PRINT_H

#include "walker.h"
#include "../common/string.h"

namespace yang {

class AstPrinter : public ConstAstWalker<y::string> {
public:

  AstPrinter();

protected:

  void preorder(const Node& node) override;
  void infix(const Node& node, const result_list& results) override;
  y::string visit(const Node& node, const result_list& results) override;

private:

  y::string indent() const;
  y::size _indent;

};

// End namespace yang.
}

#endif
