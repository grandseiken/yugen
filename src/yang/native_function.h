#ifndef YANG__NATIVE_FUNCTION_H
#define YANG__NATIVE_FUNCTION_H

#include "../common/function.h"
#include "../common/memory.h"
#include "type.h"

namespace yang {
namespace internal {

// Class for dynamic storage of an arbitrary function.
template<typename T>
class NativeFunction {
  static_assert(sizeof(T) != sizeof(T),
                "incorrect native function type argument used");
};

template<>
class NativeFunction<void> {
public:

  virtual ~NativeFunction() {}
  // Convert this to a particular NativeFunction of a given type and return the
  // contained function. It is the caller's responsibility to ensure the
  // template arguments are correct; i.e. that the dynamic type of this object
  // really is a NativeFunction<R(Args...)>.
  template<typename R, typename... Args>
  const y::function<R(Args...)>& get() const;

};

// Structure containing information about an arbitrary native function to be
// made available via a Context.
struct GenericNativeFunction {
  GenericNativeFunction()
    : ptr(y::null) {}
  ~GenericNativeFunction() {}

  GenericNativeFunction(GenericNativeFunction&&) = default;
  GenericNativeFunction& operator=(GenericNativeFunction&&) = default;

  yang::Type type;
  y::unique<NativeFunction<void>> ptr;
  y::void_fp trampoline_ptr;
};

template<typename R, typename... Args>
class NativeFunction<R(Args...)> : public NativeFunction<void> {
public:

  typedef y::function<R(Args...)> function_type;
  NativeFunction(const function_type& function);
  ~NativeFunction() override {}

private:

  friend NativeFunction<void>;
  function_type _function;

};

template<typename R, typename... Args>
NativeFunction<R(Args...)>::NativeFunction(const function_type& function)
  : _function(function)
{
}

template<typename R, typename... Args>
const y::function<R(Args...)>& NativeFunction<void>::get() const
{
  return ((NativeFunction<R(Args...)>*)this)->_function;
}

// End namespace yang::internal.
}
}

#endif
