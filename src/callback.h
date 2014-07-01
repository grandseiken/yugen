#ifndef CALLBACK_H
#define CALLBACK_H

#include <cstdint>
#include <functional>
#include <unordered_map>

template<typename... A>
class CallbackSet {
public:

  typedef std::function<void(A...)> callback;
  CallbackSet();

  std::int32_t add(const callback& c);
  void remove(std::int32_t id);
  void operator()(A... args) const;

private:

  std::int32_t _ids;
  std::unordered_map<std::int32_t, callback> _callbacks;

};

template<typename... A>
CallbackSet<A...>::CallbackSet()
  : _ids(0)
{
}

template<typename... A>
std::int32_t CallbackSet<A...>::add(const callback& c)
{
  std::int32_t id = _ids++;
  _callbacks[id] = c;
  return id;
}

template<typename... A>
void CallbackSet<A...>::remove(std::int32_t id)
{
  _callbacks.erase(id);
}

template<typename... A>
void CallbackSet<A...>::operator()(A... args) const
{
  for (const auto& pair : _callbacks) {
    pair.second(args...);
  }
}

#endif
