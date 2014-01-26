#ifndef YANG__PIPELINE_H
#define YANG__PIPELINE_H

#include "../common/map.h"
#include "../common/memory.h"
#include "../common/string.h"
#include "../common/utility.h"
#include "../common/vector.h"
#include "../log.h"

#include "native.h"
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
  template<typename...>
  struct InstanceCheck;
}

// General directions:
// TODO: Context composition; the standard libraries should be kinds of Context;
// and user types should be possible.
// TODO: there should be a type corresponding to Instance, with built-in
// functionality. Possible some sort of duck-typing interface for Instances. The
// same typing mechanism should apply to user types.
// TODO: add some kind of built-in data structures, including at least a generic
// map<K, V> type. May require garbage-collection, unless we place tight
// restrictions on their usage (e.g. only global variables).
// TODO: add a LuaValue-like generic value class.
// TODO: possibly implement closures, if it seems feasible.
// TODO: vectorised assignment, or pattern-matching assignment? Also, indexed
// assignment.
// TODO: warnings: for example, unused variables.
// TODO: many calls to log_err should probably actually be thrown exceptions.
// TODO: code hot-swapping. Careful with pointer values (e.g. functions) in
// global data struct which probably need to be left as default values.
class Context : public y::no_copy {
public:

  // Add a user type. Types must be registered before registering functions that
  // make use of them.
  template<typename T>
  void register_type(const y::string& name);

  // Add a globally-available function to the context.
  template<typename R, typename... Args>
  void register_function(
      const y::string& name, const y::function<R(Args...)>& f);

  // Information about registered types.
  template<typename T>
  bool has_type() const;
  template<typename T>
  y::string get_type_name() const;

private:

  typedef y::map<y::string, internal::GenericNativeType> type_map;
  typedef y::map<y::string, internal::GenericNativeFunction> function_map;

  friend class Program;
  const type_map& get_types();
  const function_map& get_functions() const;

  type_map _types;
  function_map _functions;

};

class Program : public y::no_copy {
public:

  Program(const Context& context, const y::string& name,
          const y::string& contents, bool optimise = true);
  ~Program();

  const Context& get_context() const;
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

  const Context& _context;
  y::string _name;
  y::unique<internal::Node> _ast;

  symbol_table _functions;
  symbol_table _globals;

  llvm::Module* _module;
  y::unique<llvm::ExecutionEngine> _engine;
  y::map<Type, llvm::Function*> _trampoline_map;

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

  template<typename>
  friend class Function;

  y::void_fp get_native_fp(const y::string& name) const;
  y::void_fp get_native_fp(llvm::Function* ir_fp) const;

  template<typename R, typename... Args>
  R call_via_trampoline(const y::string& name, const Args&... args) const;
  template<typename R, typename... Args>
  R call_via_trampoline(y::void_fp target, const Args&... args) const;

  // Runtime check that global exists and has the correct type and, if it is to
  // be modified, that it is both exported and non-const.
  bool check_global(const y::string& name, const Type& type,
                    bool for_modification) const;
  // Similarly for functions.
  bool check_function(const y::string& name, const Type& type) const;

  const Program& _program;
  void* _global_data;

};

// Implementation of Function::operator(), which has to see the defintion of
// Instance.
template<typename R, typename... Args>
R Function<R(Args...)>::operator()(const Args&... args) const
{
  // Instance should always be non-null.
  internal::ValueConstruct<R> construct;
  if (!*this) {
    log_err(_instance->get_program().get_name(),
            ": called null function object");
    return construct(*_instance);
  }
  return _instance->call_via_trampoline<R>(_function, args...);
}

// Implementation of TypeInfo for user types, which has to see the definition of
// Context.
namespace internal {
  template<typename T>
  struct TypeInfo<T*> {
    yang::Type operator()(const Context& context) const
    {
      if (!context.has_type<T>()) {
        log_err("using unregistered user type");
        return {};
      }
      yang::Type t;
      t._base = yang::Type::USER_TYPE;
      t._user_type_name = context.get_type_name<T>();
      return t;
    }
  };
}

template<typename T>
void Context::register_type(const y::string& name)
{
  auto it = _types.find(name);
  if (it != _types.end()) {
    log_err("duplicate type `", name, "` registered in context");
    return;
  }
  // Could also store a reverse-map from internal pointer type-id; probably
  // not a big deal.
  for (const auto& pair : _types) {
    if (pair.second.obj->is<T*>()) {
      log_err("duplicate types `", name, "` and `", pair.first,
              "` registered in context");
      return;
    }
  }

  internal::GenericNativeType& symbol = _types[name];
  symbol.obj = y::move_unique(new internal::NativeType<T*>());
}

template<typename R, typename... Args>
void Context::register_function(
    const y::string& name, const y::function<R(Args...)>& f)
{
  auto it = _functions.find(name);
  if (it != _functions.end()) {
    log_err("duplicate function `", name, "` registered in context");
    return;
  }

  internal::TypeInfo<Function<R(Args...)>> info;
  internal::GenericNativeFunction& symbol = _functions[name];
  symbol.type = info(*this);
  symbol.ptr = y::move_unique(new internal::NativeFunction<R(Args...)>(f));

  symbol.trampoline_ptr = (y::void_fp)&internal::ReverseTrampolineCall<
      R(Args...),
      typename internal::TrampolineReturn<R>::type,
      typename internal::TrampolineArgs<Args...>::type>::call;
}

template<typename T>
bool Context::has_type() const
{
  for (const auto& pair : _types) {
    if (pair.second.obj->is<T*>()) {
      return true;
    }
  }
  return false;
}

template<typename T>
y::string Context::get_type_name() const
{
  for (const auto& pair : _types) {
    if (pair.second.obj->is<T*>()) {
      return pair.first;
    }
  }
  return "";
}

template<typename T>
T Instance::get_global(const y::string& name) const
{
  // TypeInfo will fail at compile-time for completely unsupported types; will
  // at runtime for pointers to unregistered user types.
  internal::TypeInfo<T> info;
  if (!check_global(name, info(_program.get_context()), false)) {
    return T();
  }
  return call_via_trampoline<T>("!global_get_" + name);
}

template<typename T>
void Instance::set_global(const y::string& name, const T& value)
{
  internal::TypeInfo<T> info;
  if (!check_global(name, info(_program.get_context()), true)) {
    return;
  }
  call_via_trampoline<void>("!global_set_" + name, value);
}

template<typename T>
T Instance::get_function(const y::string& name)
{
  internal::TypeInfo<T> info;
  internal::ValueConstruct<T> construct;
  T result = construct(*this);
  if (!check_function(name, info(_program.get_context()))) {
    return result;
  }
  construct.set_void_fp(result, get_native_fp(name));
  return result;
}

template<typename R, typename... Args>
R Instance::call(const y::string& name, const Args&... args)
{
  internal::TypeInfo<Function<R(Args...)>> info;
  internal::ValueConstruct<R> construct;
  if (!check_function(name, info(_program.get_context()))) {
    return construct(*this);
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
R Instance::call_via_trampoline(y::void_fp target, const Args&... args) const
{
  // Make sure only functions referencing this instance are passed in.
  internal::InstanceCheck<Args...> instance_check;
  internal::ValueConstruct<R> construct;
  if (!instance_check(*this, args...)) {
    return construct(const_cast<Instance&>(*this));
  }

  // Since we can only obtain a valid Function object referencing a function
  // type for which a trampoline has been generated, there should always be
  // an entry in the trampoline map.
  auto it = _program._trampoline_map.find(
      Function<R(Args...)>::get_type(_program.get_context()));
  y::void_fp trampoline = get_native_fp(it->second);

  typedef internal::TrampolineCall<R, void*, Args..., y::void_fp> call_type;
  typename call_type::fp_type trampoline_expanded =
      (typename call_type::fp_type)trampoline;
  return call_type()(const_cast<Instance&>(*this),
                     trampoline_expanded, _global_data, args..., target);
}

// End namespace yang.
}

#endif
