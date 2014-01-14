#ifndef YANG__PIPELINE_H
#define YANG__PIPELINE_H

#include "../common/map.h"
#include "../common/memory.h"
#include "../common/string.h"
#include "../common/utility.h"
#include "../common/vector.h"

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
// TODO: create Instances from Programs, and use them to run instances of the
// programs.
// TODO: create Programs from Contexts or nvironments which define game-library
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

private:

  void* get_native_fp(const y::string& name);

  const Program& _program;
  void* _global_data;

};

// End namespace yang.
}

#endif
