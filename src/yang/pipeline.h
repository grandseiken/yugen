#ifndef YANG__PIPELINE_H
#define YANG__PIPELINE_H

#include "../common/map.h"
#include "../common/memory.h"
#include "../common/string.h"
#include "../common/vector.h"

#include "type.h"

namespace llvm {
  class Module;
  class ExecutionEngine;
  class Type;
}

namespace yang {

struct Node;

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
class Program {
public:

  Program(const y::string& name, const y::string& contents);
  ~Program();

  // Returns true if the contents parsed and checked successfully. Otherwise,
  // none of the following functions will do anything useful.
  bool success() const;
  y::string print_ast() const;

  void generate_ir();
  void optimise_ir();
  y::string print_ir() const;

private:

  y::string _name;
  y::string _error;

  y::unique<Node> _ast;
  y::map<y::string, Type> _globals;

  y::unique<llvm::Module> _module;
  llvm::ExecutionEngine* _engine;

};

struct ParseGlobals {
  static const y::string* lexer_input_contents;
  static y::size lexer_input_offset;

  static Node* parser_output;
  static y::vector<y::string> errors;
  static y::string error(
      y::size line, const y::string& token, const y::string& message);
};

// End namespace yang.
}

#endif
