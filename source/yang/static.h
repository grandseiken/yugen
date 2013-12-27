#ifndef YANG__STATIC_H
#define YANG__STATIC_H

#include "walker.h"

// TYPE_ERROR is a special type assigned to expressions containing an error
// where the type cannot be determined. Further errors involving a subtree
// of this type are suppressed (to avoid cascading error messages).
enum Type {
  TYPE_ERROR,
  TYPE_INT,
  TYPE_WORLD,
};

class StaticChecker : public ConstAstWalker<Type> {
public:

  StaticChecker();
  bool errors() const;

protected:

  Type visit(const Node& node, const result_list& results) override;

private:

  void error(const Node& node, const y::string& message);

  bool _errors;

};

#endif
