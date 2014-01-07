#ifndef YANG__PIPELINE_H
#define YANG__PIPELINE_H

#include "../common/map.h"
#include "../common/memory.h"
#include "../common/string.h"
#include "../common/vector.h"

#include "type.h"

struct Node;
namespace llvm {
  class Module;
  class ExecutionEngine;
  class Type;
}

// General directions:
// TODO: create YangInstances from YangPrograms, and use them to run instances
// of the programs.
// TODO: create YangPrograms from YangContexts or YangEnvironments which define
// game-library functions and types. (Perhaps the standard library can be a kind
// of YangContext, even.)
// TODO: add some kind of built-in data structures, including at least a generic
// map<K, V> type. May require garbage-collection, unless we place tight
// restrictions on their usage (e.g. only global variables).
// TODO: allow some kind of function recursion. This probably has to involve a
// hack where an assignment (local or top-level) with a function-expression on
// the immediate right-hand side causes it to be entered in the symbol-table
// before the function body is processed. It would be nice if global functions
// could be mutually-recursive, too, but that requires a two-pass approach.
// TODO: possibly implement closures, if it seems feasible.
class YangProgram {
public:

  YangProgram(const y::string& name, const y::string& contents);
  ~YangProgram();

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

#endif
