#ifndef YANG__PIPELINE_H
#define YANG__PIPELINE_H

#include "../common/map.h"
#include "../common/memory.h"
#include "../common/string.h"
#include "../common/utility.h"
#include "../common/vector.h"
#include "../log.h"

#include "type.h"

namespace llvm {
  class Module;
  class ExecutionEngine;
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
// TODO: possibly implement closures, if it seems feasible.
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

  typedef y::map<y::string, Type> symbol_table;
  const symbol_table& get_functions() const;
  const symbol_table& get_globals() const;

private:

  friend class Instance;

  void generate_ir();
  void optimise_ir();

  symbol_table _functions;
  symbol_table _globals;

  y::string _name;
  y::unique<internal::Node> _ast;
  y::unique<llvm::Module> _module;
  llvm::ExecutionEngine* _engine;

};

class Instance : public y::no_copy {
public:

  Instance(const Program& program);
  ~Instance();

  // TODO: type-checking with some sort of templated yang::TypeInfo metadata
  // struct.
  template<typename Type>
  Type get_global(const y::string& name) const;
  template<typename Type>
  void set_global(const y::string& name, const Type& value);
  // TODO: call functions.

private:

  void* get_native_fp(const y::string& name) const;

  const Program& _program;
  void* _global_data;

};

template<typename Type>
Type Instance::get_global(const y::string& name) const
{
  if (_program._globals.find(name) == _program._globals.end()) {
    log_err(_program._name +
            ": requested global `" + name + "` does not exist");
    return Type();
  }
  void* native = get_native_fp("!global_get_" + name);
  return ((Type(*)(void*))native)(_global_data);
}

template<typename Type>
void Instance::set_global(const y::string& name, const Type& value)
{
  if (_program._globals.find(name) == _program._globals.end()) {
    log_err(_program._name +
            ": requested global `" + name + "` does not exist");
  }
  else {
    void* native = get_native_fp("!global_set_" + name);
    ((void(*)(void*, Type))native)(_global_data, value);
  }
}

// End namespace yang.
}

#endif
