#include "pipeline.h"

#include "ast.h"
#include "print.h"
#include "static.h"
#include "../log.h"
#include "../../gen/yang/yang.y.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/PassManager.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Support/TargetSelect.h>

int yyparse();

YangProgram::YangProgram(const y::string& contents)
  : _ast(y::null)
{
  ParseGlobals::lexer_input_contents = &contents;
  ParseGlobals::lexer_input_offset = 0;
  ParseGlobals::parser_output = y::null;
  ParseGlobals::errors.clear();

  yyparse();
  y::unique<Node> output = y::move_unique(ParseGlobals::parser_output);
  Node::orphans.erase(output.get());
  for (Node* node : Node::orphans) {
    y::move_unique(node);
  }
  Node::orphans.clear();

  for (const y::string& s : ParseGlobals::errors) {
    log_err(s);
  }
  if (ParseGlobals::errors.size()) {
    return;
  }

  StaticChecker checker;
  checker.walk(*output);
  if (checker.errors()) {
    return;
  }
  _ast = y::move_unique(output);
}

YangProgram::~YangProgram()
{
}

bool YangProgram::success() const
{
  return bool(_ast);
}

y::string YangProgram::print_ast() const
{
  if (!success()) {
    return "<error>";
  }
  AstPrinter printer;
  return printer.walk(*_ast);
}

const y::string* ParseGlobals::lexer_input_contents = y::null;
y::size ParseGlobals::lexer_input_offset = 0;

Node* ParseGlobals::parser_output = y::null;
y::vector<y::string> ParseGlobals::errors;
