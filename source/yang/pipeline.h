#ifndef YANG__PIPELINE_H
#define YANG__PIPELINE_H

#include "../common/memory.h"
#include "../common/string.h"
#include "../common/vector.h"

struct Node;

class YangProgram {
public:

  YangProgram(const y::string& contents);
  ~YangProgram();

  bool success() const;
  y::string print_ast() const;

private:

  y::unique<Node> _ast;

};

struct ParseGlobals {
  static const y::string* lexer_input_contents;
  static y::size lexer_input_offset;

  static Node* parser_output;
  static y::vector<y::string> errors;
};

#endif
