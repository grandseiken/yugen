#ifndef YANG__TABLE_H
#define YANG__TABLE_H

#include "../common/map.h"
#include "../common/math.h"

template<typename T>
class SymbolTable {
public:

  SymbolTable();
  explicit SymbolTable(const T& default_value);

  void push();
  void pop();
  y::size size() const;

  bool add(const y::string& symbol, const T& t);
  void remove(const y::string& symbol);

  bool has(const y::string& symbol) const;
  const T& operator[](const y::string& symbol) const;
  /***/ T& operator[](const y::string& symbol);

private:

  T _default;
  typedef y::map<y::string, T> scope;
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
bool SymbolTable<T>::add(const y::string& symbol, const T& t)
{
  _stack.rbegin()->emplace(symbol, t);
}

template<typename T>
void SymbolTable<T>::remove(const y::string& symbol)
{
  _stack.rbegin()->erase(symbol);
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

#endif
