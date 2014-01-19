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
// TODO: vectorised assignment, or pattern-matching assignment?
// TODO: warnings: for example, unused variables.
class Program : public y::no_copy {
public:

  Program(const y::string& name,
          const y::string& contents, bool optimise = true);
  ~Program();

  // Returns true if the contents parsed and checked successfully. Otherwise,
  // none of the following functions will do anything useful.
  bool success() const;
  y::string print_ast() const;
  y::string print_ir() const;

  // Functions at the global scope are differentiated from global variables
  // having a function type. Currently only global-scope functions can be
  // called.
  // TODO: with trampoline entrypoints there isn't really much reason for
  // this limitation any more (except that finding the correct trampoline
  // might be a challenge?).
  typedef y::map<y::string, Type> symbol_table;
  const symbol_table& get_functions() const;
  const symbol_table& get_globals() const;

private:

  friend class Instance;

  void generate_ir();
  void optimise_ir();

  symbol_table _functions;
  symbol_table _globals;
  y::map<y::string, llvm::Function*> _trampoline_map;

  y::string _name;
  y::unique<internal::Node> _ast;
  y::unique<llvm::Module> _module;
  llvm::ExecutionEngine* _engine;

};

class Instance : public y::no_copy {
public:

  Instance(const Program& program);
  ~Instance();

  template<typename T>
  T get_global(const y::string& name) const;
  template<typename T>
  void set_global(const y::string& name, const T& value);

  template<typename R, typename... Args>
  R call(const y::string& name, const Args&... args);

private:

  typedef void (*void_fp)();
  void_fp get_native_fp(const y::string& name) const;
  void_fp get_native_fp(llvm::Function* ir_fp) const;

  template<typename R, typename... Args>
  R call_via_trampoline(const y::string& name, const Args&... args) const;

  // Runtime check that global exists and has the correct type and, if it is to
  // be modified, that it is both exported and non-const.
  bool check_global(const y::string& name, const Type& type,
                    bool for_modification) const;
  // Similarly for functions.
  bool check_function(const y::string& name, const Type& type) const;

  const Program& _program;
  void* _global_data;

};

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
  auto it = _program._trampoline_map.find(name);
  void_fp trampoline = get_native_fp(it->second);
  void_fp target = get_native_fp(name);

  typedef internal::TrampolineCall<R, void*, Args..., void_fp> call_type;
  typename call_type::fp_type trampoline_expanded =
      (typename call_type::fp_type)trampoline;
  return call_type()(trampoline_expanded, _global_data, args..., target);
}

// End namespace yang.
}

#endif
