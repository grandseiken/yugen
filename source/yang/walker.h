#ifndef YANG__WALKER_H
#define YANG__WALKER_H

#include "ast.h"
#include "../common/pair.h"

template<bool Const>
struct AstWalkerNodeType {};
template<>
struct AstWalkerNodeType<false> {
  typedef Node type;
};
template<>
struct AstWalkerNodeType<true> {
  typedef const Node type;
};

template<typename T, bool Const>
class AstWalkerBase {
public:

  typedef typename AstWalkerNodeType<Const>::type N;
  void walk(N& node);

  virtual T visit(N& node) = 0;

private:

  y::vector<T> _results;

};

template<typename T, bool Const>
void AstWalkerBase<T, Const>::walk(N& node)
{
  typedef y::pair<N*, decltype(node.children.begin())> pair;
  y::vector<pair> stack;

  stack.emplace_back(&node, node.children.begin());
  while (!stack.empty()) {
    N* n = stack.rbegin()->first;
    auto& it = stack.rbegin()->second;

    if (it == n->children.end()) {
      visit(*n);
      stack.pop_back();
      continue;
    }

    N& next = **(it++);
    stack.emplace_back(&next, next.children.begin());
  }
}

template<typename T>
using AstWalker = AstWalkerBase<T, false>;
template<typename T>
using ConstAstWalker = AstWalkerBase<T, true>;

#endif
