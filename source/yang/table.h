#ifndef YANG__TABLE_H
#define YANG__TABLE_H

#include "../common/map.h"
#include "../common/math.h"
#include "../common/set.h"
#include "../common/string.h"
#include "../common/vector.h"

template<typename T>
class SymbolTable {
public:

  SymbolTable();
  explicit SymbolTable(const T& default_value);

  // Push or pop a frame from the symbol table.
  void push();
  void pop();
  // Depth of frames in the symbol table. Guaranteed to be at least 1.
  y::size size() const;

  // Add or remove from the top frame.
  void add(const y::string& symbol, const T& t);
  void remove(const y::string& symbol);

  // Add or remove from an arbitrary frame.
  void add(const y::string& symbol, y::size frame, const T& t);
  void remove(const y::string& symbol, y::size frame);

  // Symbol existence for whole table or a particular frame.
  bool has(const y::string& symbol) const;
  bool has(const y::string& symbol, y::size frame) const;

  // Get list of symbols in a particular frame-range [min_frame, max_frame).
  void get_symbols(y::set<y::string>& output,
                   y::size min_frame, y::size max_frame) const;

  // Get index of the topmost frame in which the symbol is defined.
  y::size index(const y::string& symbol) const;

  // Value retrieval.
  const T& operator[](const y::string& symbol) const;
  /***/ T& operator[](const y::string& symbol);
  const T& get(const y::string& symbol, y::size frame) const;
  /***/ T& get(const y::string& symbol, y::size frame);

  typedef y::map<y::string, T> scope;
  const scope& frame(y::size frame) const;
  const scope& top() const;

private:

  T _default;
  y::vector<scope> _stack;

};

template<typename T>
SymbolTable<T>::SymbolTable()
{
  push();
}

template<typename T>
SymbolTable<T>::SymbolTable(const T& default_value)
  : _default(default_value)
{
  push();
}

template<typename T>
void SymbolTable<T>::push()
{
  _stack.emplace_back();
}

template<typename T>
void SymbolTable<T>::pop()
{
  if (size() > 1) {
    _stack.pop_back();
  }
}

template<typename T>
y::size SymbolTable<T>::size() const
{
  return _stack.size();
}

template<typename T>
void SymbolTable<T>::add(const y::string& symbol, const T& t)
{
  _stack.rbegin()->emplace(symbol, t);
}

template<typename T>
void SymbolTable<T>::remove(const y::string& symbol)
{
  _stack.rbegin()->erase(symbol);
}

template<typename T>
void SymbolTable<T>::add(const y::string& symbol, y::size frame, const T& t)
{
  if (frame < size()) {
    _stack[frame].emplace(symbol, t);
  }
}

template<typename T>
void SymbolTable<T>::remove(const y::string& symbol, y::size frame)
{
  if (frame < size()) {
    _stack[frame].erase(symbol);
  }
}

template<typename T>
bool SymbolTable<T>::has(const y::string& symbol) const
{
  for (auto it = _stack.rbegin(); it != _stack.rend(); ++it) {
    if (it->find(symbol) != it->end()) {
      return true;
    }
  }
  return false;
}

template<typename T>
bool SymbolTable<T>::has(const y::string& symbol, y::size frame) const
{
  return frame < size() && _stack[frame].find(symbol) != _stack[frame].end();
}

template<typename T>
void SymbolTable<T>::get_symbols(y::set<y::string>& output,
                                 y::size min_frame, y::size max_frame) const
{
  for (y::size i = min_frame; i < max_frame && i < size(); ++i) {
    for (const auto& pair : _stack[i]) {
      output.insert(pair.first);
    }
  }
}

template<typename T>
y::size SymbolTable<T>::index(const y::string& symbol) const
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

template<typename T>
const T& SymbolTable<T>::operator[](const y::string& symbol) const
{
  for (auto it = _stack.rbegin(); it != _stack.rend(); ++it) {
    auto jt = it->find(symbol);
    if (jt != it->end()) {
      return jt->second;
    }
  }
  return _default;
}

template<typename T>
T& SymbolTable<T>::operator[](const y::string& symbol)
{
  for (auto it = _stack.rbegin(); it != _stack.rend(); ++it) {
    auto jt = it->find(symbol);
    if (jt != it->end()) {
      return jt->second;
    }
  }
  return _default;
}

template<typename T>
const T& SymbolTable<T>::get(const y::string& symbol, y::size frame) const
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

template<typename T>
T& SymbolTable<T>::get(const y::string& symbol, y::size frame)
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

template<typename T>
const typename SymbolTable<T>::scope& SymbolTable<T>::frame(
    const y::size frame) const
{
  if (frame >= size()) {
    return top();
  }
  return _stack[frame];
}

template<typename T>
const typename SymbolTable<T>::scope& SymbolTable<T>::top() const
{
  return *_stack.rbegin();
}

#endif
