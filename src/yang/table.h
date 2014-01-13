#ifndef YANG__TABLE_H
#define YANG__TABLE_H

#include "../common/map.h"
#include "../common/math.h"
#include "../common/set.h"
#include "../common/vector.h"

namespace yang {

template<typename K, typename V>
class SymbolTable {
public:

  SymbolTable();
  explicit SymbolTable(const V& default_value);

  // Push or pop a frame from the symbol table.
  void push();
  void pop();
  // Depth of frames in the symbol table. Guaranteed to be at least 1.
  y::size size() const;

  // Add or remove from the top frame.
  void add(const K& symbol, const V& v);
  void remove(const K& symbol);

  // Add or remove from an arbitrary frame.
  void add(const K& symbol, y::size frame, const V& v);
  void remove(const K& symbol, y::size frame);

  // Symbol existence for whole table or a particular frame.
  bool has(const K& symbol) const;
  bool has(const K& symbol, y::size frame) const;

  // Get list of symbols in a particular frame-range [min_frame, max_frame).
  void get_symbols(y::set<K>& output,
                   y::size min_frame, y::size max_frame) const;

  // Get index of the topmost frame in which the symbol is defined.
  y::size index(const K& symbol) const;

  // Value retrieval.
  const V& operator[](const K& symbol) const;
  /***/ V& operator[](const K& symbol);
  const V& get(const K& symbol, y::size frame) const;
  /***/ V& get(const K& symbol, y::size frame);

private:

  V _default;
  typedef y::map<K, V> scope;
  y::vector<scope> _stack;

};

template<typename K, typename V>
SymbolTable<K, V>::SymbolTable()
{
  push();
}

template<typename K, typename V>
SymbolTable<K, V>::SymbolTable(const V& default_value)
  : _default(default_value)
{
  push();
}

template<typename K, typename V>
void SymbolTable<K, V>::push()
{
  _stack.emplace_back();
}

template<typename K, typename V>
void SymbolTable<K, V>::pop()
{
  if (size() > 1) {
    _stack.pop_back();
  }
}

template<typename K, typename V>
y::size SymbolTable<K, V>::size() const
{
  return _stack.size();
}

template<typename K, typename V>
void SymbolTable<K, V>::add(const K& symbol, const V& v)
{
  _stack.rbegin()->emplace(symbol, v);
}

template<typename K, typename V>
void SymbolTable<K, V>::remove(const K& symbol)
{
  _stack.rbegin()->erase(symbol);
}

template<typename K, typename V>
void SymbolTable<K, V>::add(const K& symbol, y::size frame, const V& v)
{
  if (frame < size()) {
    _stack[frame].emplace(symbol, v);
  }
}

template<typename K, typename V>
void SymbolTable<K, V>::remove(const K& symbol, y::size frame)
{
  if (frame < size()) {
    _stack[frame].erase(symbol);
  }
}

template<typename K, typename V>
bool SymbolTable<K, V>::has(const K& symbol) const
{
  for (auto it = _stack.rbegin(); it != _stack.rend(); ++it) {
    if (it->find(symbol) != it->end()) {
      return true;
    }
  }
  return false;
}

template<typename K, typename V>
bool SymbolTable<K, V>::has(const K& symbol, y::size frame) const
{
  return frame < size() && _stack[frame].find(symbol) != _stack[frame].end();
}

template<typename K, typename V>
void SymbolTable<K, V>::get_symbols(y::set<K>& output,
                                    y::size min_frame, y::size max_frame) const
{
  for (y::size i = min_frame; i < max_frame && i < size(); ++i) {
    for (const auto& pair : _stack[i]) {
      output.insert(pair.first);
    }
  }
}

template<typename K, typename V>
y::size SymbolTable<K, V>::index(const K& symbol) const
{
  y::size i = size();
  while (i) {
    --i;
    auto jt = _stack[i].find(symbol);
    if (jt != _stack[i].end()) {
      return i;
    }
  }
  return _stack.size();
}

template<typename K, typename V>
const V& SymbolTable<K, V>::operator[](const K& symbol) const
{
  for (auto it = _stack.rbegin(); it != _stack.rend(); ++it) {
    auto jt = it->find(symbol);
    if (jt != it->end()) {
      return jt->second;
    }
  }
  return _default;
}

template<typename K, typename V>
V& SymbolTable<K, V>::operator[](const K& symbol)
{
  for (auto it = _stack.rbegin(); it != _stack.rend(); ++it) {
    auto jt = it->find(symbol);
    if (jt != it->end()) {
      return jt->second;
    }
  }
  return _default;
}

template<typename K, typename V>
const V& SymbolTable<K, V>::get(const K& symbol, y::size frame) const
{
  if (frame >= size()) {
    return _default;
  }
  auto it = _stack[frame].find(symbol);
  if (it != _stack[frame].end()) {
    return it->second;
  }
  return _default;
}

template<typename K, typename V>
V& SymbolTable<K, V>::get(const K& symbol, y::size frame)
{
  if (frame >= size()) {
    return _default;
  }
  auto it = _stack[frame].find(symbol);
  if (it != _stack[frame].end()) {
    return it->second;
  }
  return _default;
}

// End namespace yang.
}

#endif