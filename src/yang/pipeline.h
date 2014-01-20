#ifndef YANG__PIPELINE_H
#define YANG__PIPELINE_H

#include "../common/map.h"
#include "../common/memory.h"
#include "../common/string.h"
#include "../common/utility.h"
#include "../common/vector.h"
#include "../log.h"

#include "type.h"
#include "type_info.h"

namespace llvm {
  class ExecutionEngine;
  class Function;
  class Module;
  class Type;
}

namespace yang {

namespace internal {
  struct Node;
  template<typename... Args>
  struct InstanceCheck;
}

// General directions:
// TODO: create Programs from Contexts or Environments which define game-library
// functions and types. (Perhaps the standard library can be a kind of Context,
// even.)
// TODO: in particular, there should be a type corresponding to Instance, with
// built-in functionality.
// TODO: add some kind of built-in data structures, including at least a generic
// map<K, V> type. May require garbage-collection, unless we place tight
// restrictions on their usage (e.g. only global variables).
// TODO: add a LuaValue-like generic value class.
// TODO: possibly implement closures, if it seems feasible.
// TODO: vectorised assignment, or pattern-matching assignment? Also, indexed
// assignment.
// TODO: warnings: for example, unused variables.
class Program : public y::no_copy {
public:

  Program(const y::string& name,
          const y::string& contents, bool optimise = true);
  ~Program();

  const y::string& get_name() const;

  // Returns true if the contents parsed and checked successfully. Otherwise,
  // none of the following functions will do anything useful.
  bool success() const;
  y::string print_ast() const;
  y::string print_ir() const;

  typedef y::map<y::string, Type> symbol_table;
  const symbol_table& get_functions() const;
  const symbol_table& get_globals() const;

private:

  friend class Instance;

  void generate_ir();
  void optimise_ir();

  symbol_table _functions;
  symbol_table _globals;
  y::map<Type, llvm::Function*> _trampoline_map;

  y::string _name;
  y::unique<internal::Node> _ast;
  y::unique<llvm::Module> _module;
  llvm::ExecutionEngine* _engine;

};

class Instance : public y::no_copy {
public:

  Instance(const Program& program);
  ~Instance();

  const Program& get_program() const;

  template<typename T>
  T get_global(const y::string& name) const;
  template<typename T>
  void set_global(const y::string& name, const T& value);

  template<typename R, typename... Args>
  R call(const y::string& name, const Args&... args);
  template<typename T>
  T get_function(const y::string& name);

private:

  template<typename R, typename... Args>
  friend class Function;

  typedef void (*void_fp)();
  void_fp get_native_fp(const y::string& name) const;
  void_fp get_native_fp(llvm::Function* ir_fp) const;

  template<typename R, typename... Args>
  R call_via_trampoline(const y::string& name, const Args&... args) const;
  template<typename R, typename... Args>
  R call_via_trampoline(void_fp target, const Args&... args) const;

  // Runtime check that global exists and has the correct type and, if it is to
  // be modified, that it is both exported and non-const.
  bool check_global(const y::string& name, const Type& type,
                    bool for_modification) const;
  // Similarly for functions.
  bool check_function(const y::string& name, const Type& type) const;

  const Program& _program;
  void* _global_data;

};

namespace internal {

// Check that all Functions given in an argument list match some particular
// program instance.
template<typename... Args>
struct InstanceCheck {};
template<>
struct InstanceCheck<> {
  bool operator()(const Instance& instance) const
  {
    (void)instance;
    return true;
  }
};

template<typename A, typename... Args>
struct InstanceCheck<A, Args...> {
  bool operator()(const Instance& instance,
                  const A& arg, const Args&... args) const
  {
    (void)arg;
    InstanceCheck<Args...> next;
    return next(instance, args...);
  }
};

template<typename FR, typename... FArgs, typename... Args>
struct InstanceCheck<Function<FR, FArgs...>, Args...> {
  bool operator()(const Instance& instance,
                  const Function<FR, FArgs...>& arg, const Args&... args) const
  {
    bool result = true;
    InstanceCheck<Args...> next;
    if (!arg.is_valid()) {
      log_err(instance.get_program().get_name(), ": passed null function");
      result = false;
    }
    else {
      if (&arg.get_instance().get_program() != &instance.get_program()) {
        log_err(instance.get_program().get_name(),
                ": passed function referencing different program ",
                 arg.get_instance().get_program().get_name());
        result = false;
      }
      if (&arg.get_instance() != &instance) {
        log_err(instance.get_program().get_name(),
                ": passed function referencing different program instance");
        result = false;
      }
    }
    return next(instance, args...) && result;
  }
};

// End namespace internal.
}

// Implementation of Function::call, which has to see the defintion of Instance.
template<typename R, typename... Args>
R Function<R, Args...>::call(const Args&... args) const
{
  // TODO: change FunctionTypeInfo so that we can always set the Instance of a
  // null Function and at least print the program name.
  if (!is_valid()) {
    log_err("called null function");
    return R();
  }
  return _instance->call_via_trampoline<R>(_function, args...);
}

template<typename T>
T Instance::get_global(const y::string& name) const
{
  // TypeInfo representation() will fail at compile-time for completely
  // unsupported types.
  internal::TypeInfo<T> info;
  if (!check_global(name, info(), false)) {
    return T();
  }
  return call_via_trampoline<T>("!global_get_" + name);
}

template<typename T>
void Instance::set_global(const y::string& name, const T& value)
{
  internal::TypeInfo<T> info;
  if (!check_global(name, info(), true)) {
    return;
  }
  call_via_trampoline<void>("!global_set_" + name, value);
}

template<typename T>
T Instance::get_function(const y::string& name)
{
  internal::TypeInfo<T> info;
  internal::FunctionTypeInfo<T> function_info;
  if (!check_function(name, info())) {
    return T();
  }
  T result;
  function_info(result, this, get_native_fp(name));
  return result;
}

template<typename R, typename... Args>
R Instance::call(const y::string& name, const Args&... args)
{
  internal::TypeInfo<Function<R, Args...>> info;
  if (!check_function(name, info())) {
    return R();
  }
  return call_via_trampoline<R>(name, args...);
}

template<typename R, typename... Args>
R Instance::call_via_trampoline(
    const y::string& name, const Args&... args) const
{
  return call_via_trampoline<R>(get_native_fp(name), args...);
}

template<typename R, typename... Args>
R Instance::call_via_trampoline(void_fp target, const Args&... args) const
{
  // Make sure only functions referencing this instance are passed in.
  internal::InstanceCheck<Args...> instance_check;
  if (!instance_check(*this, args...)) {
    return R();
  }

  // Since we can only obtain a valid Function object referencing a function
  // type for which a trampoline has been generated, there should always be
  // an entry in the trampoline map.
  auto it = _program._trampoline_map.find(Function<R, Args...>::get_type());
  void_fp trampoline = get_native_fp(it->second);

  typedef internal::TrampolineCall<R, void*, Args..., void_fp> call_type;
  typename call_type::fp_type trampoline_expanded =
      (typename call_type::fp_type)trampoline;
  return call_type()(const_cast<Instance&>(*this),
                     trampoline_expanded, _global_data, args..., target);
}

// End namespace yang.
}

#endif
