#ifndef YANG__PIPELINE_H
#define YANG__PIPELINE_H

#include "../common/memory.h"
#include "../common/string.h"
#include "../common/vector.h"

struct Node;
y::unique<Node> parse_yang_ast(const y::string& contents);
y::string print_yang_ast(const Node& node);

struct ParseGlobals {
  static const y::string* lexer_input_contents;
  static y::size lexer_input_offset;

  static Node* parser_output;
  static y::vector<y::string> errors;
};

#endif
