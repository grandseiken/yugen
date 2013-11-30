#ifndef CALLBACK_H
#define CALLBACK_H

#include "common.h"

template<typename... A>
struct Callback {
  virtual void operator()(A... args) = 0;
};

template<typename T, typename... A>
struct ObjCallback : public Callback<A...> {
  typedef void (T::*pointer)(A... args);
  ObjCallback(T& object, pointer p)
    : object(object)
    , p(p)
  {}

  T& object;
  pointer p;

  void operator()(A... args) override;
};

template<typename... A>
class CallbackSet : public y::no_copy {
public:

  typedef Callback<A...> callback;

  void add(callback* c);
  void remove(callback* c);
  void operator()(A... args) const;

private:

  y::set<callback*> _callbacks;

};

template<typename T, typename... A>
void ObjCallback<T, A...>::operator()(A... args)
{
  (object.*p)(args...);
}

template<typename... A>
void CallbackSet<A...>::add(callback* c)
{
  _callbacks.insert(c);
}

template<typename... A>
void CallbackSet<A...>::remove(callback* c)
{
  _callbacks.erase(c);
}

template<typename... A>
void CallbackSet<A...>::operator()(A... args) const
{
  for (callback* c : _callbacks) {
    (*c)(args...);
  }
}

#endif
