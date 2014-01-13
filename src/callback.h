#ifndef CALLBACK_H
#define CALLBACK_H

#include "common/function.h"
#include "common/map.h"
#include "common/math.h"
#include "common/utility.h"

template<typename... A>
class CallbackSet : public y::no_copy {
public:

  typedef y::function<void(A...)> callback;
  CallbackSet();

  y::int32 add(const callback& c);
  void remove(y::int32 id);
  void operator()(A... args) const;

private:

  y::int32 _ids;
  y::map<y::int32, callback> _callbacks;

};

template<typename... A>
CallbackSet<A...>::CallbackSet()
  : _ids(0)
{
}

template<typename... A>
y::int32 CallbackSet<A...>::add(const callback& c)
{
  y::int32 id = _ids++;
  _callbacks[id] = c;
  return id;
}

template<typename... A>
void CallbackSet<A...>::remove(y::int32 id)
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
