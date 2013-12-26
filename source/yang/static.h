#ifndef YANG__STATIC_H
#define YANG__STATIC_H

#include "walker.h"

enum Type {
  TYPE_INT,
  TYPE_WORLD,
};

class StaticChecker : public ConstAstWalker<Type> {
public:

  Type visit(const Node& node) override;

};

#endif
