#ifndef YANG__PIPELINE_H
#define YANG__PIPELINE_H

#include "../common/map.h"
#include "../common/memory.h"
#include "../common/string.h"
#include "../common/vector.h"

#include "type.h"

struct Node;
class Type;
namespace llvm {
  class Module;
  class ExecutionEngine;
  class Type;
}

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
