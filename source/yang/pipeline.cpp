#include "pipeline.h"

#include "ast.h"
#include "irgen.h"
#include "print.h"
#include "static.h"
#include "../log.h"
#include "../../gen/yang/yang.y.h"

#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/PassManager.h>

int yyparse();

YangProgram::YangProgram(const y::string& name, const y::string& contents)
  : _name(name)
  , _ast(y::null)
  , _module(y::null)
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
  _globals = checker.global_variable_map();
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
  return printer.walk(*_ast) + '\n';
}

void YangProgram::generate_ir()
{
  if (!success()) {
    return;
  }

  llvm::InitializeNativeTarget();
  _module = y::move_unique(
      new llvm::Module(_name, llvm::getGlobalContext()));

  // TODO: unsure if I am supposed to delete the ExecutionEngine, or the Values.
  _engine = llvm::EngineBuilder(_module.get()).setErrorStr(&_error).create();
  if (!_engine) {
    log_err("Couldn't create execution engine:\n", _error);
    _module = y::null;
  }

  IrGenerator irgen(*_module, _globals);
  irgen.walk(*_ast);
  irgen.emit_global_functions();

  if (llvm::verifyModule(*_module, llvm::ReturnStatusAction, &_error)) {
    log_err("Couldn't verify module:\n", _error);
  }
}

void YangProgram::optimise_ir()
{
  if (!_module) {
    return;
  }

  llvm::PassManager optimiser;
  optimiser.add(new llvm::DataLayout(*_engine->getDataLayout()));

  // Basic alias analysis and register promotion.
  optimiser.add(llvm::createBasicAliasAnalysisPass());
  optimiser.add(llvm::createPromoteMemoryToRegisterPass());

  // Optimise instructions, and reassociate for constant propagation.
  optimiser.add(llvm::createInstructionCombiningPass());
  optimiser.add(llvm::createReassociatePass());
  optimiser.add(llvm::createGVNPass());

  // Simplify the control-flow graph before tackling loops.
  optimiser.add(llvm::createCFGSimplificationPass());

  // Handle loops.
  optimiser.add(llvm::createIndVarSimplifyPass());
  optimiser.add(llvm::createLoopIdiomPass());
  optimiser.add(llvm::createLoopRotatePass());
  optimiser.add(llvm::createLoopUnrollPass());
  optimiser.add(llvm::createLoopUnswitchPass());
  optimiser.add(llvm::createLoopDeletionPass());

  // Simplify again and delete all the dead code.
  optimiser.add(llvm::createCFGSimplificationPass());
  optimiser.add(llvm::createAggressiveDCEPass());

  // Interprocedural optimisations.
  optimiser.add(llvm::createFunctionInliningPass());
  optimiser.add(llvm::createIPConstantPropagationPass());
  optimiser.add(llvm::createGlobalOptimizerPass());
  optimiser.add(llvm::createDeadArgEliminationPass());
  optimiser.add(llvm::createGlobalDCEPass());
  optimiser.add(llvm::createTailCallEliminationPass());

  // After function inlining run a few passes again.
  optimiser.add(llvm::createInstructionCombiningPass());
  optimiser.add(llvm::createReassociatePass());
  optimiser.add(llvm::createGVNPass());

  // Run the optimisation passes.
  // TODO: work out if there's others we should run, or in a different order.
  optimiser.run(*_module);
}

y::string YangProgram::print_ir() const
{
  if (!_module) {
    return "<error>";
  }
  y::string output;
  llvm::raw_string_ostream os(output);
  _module->print(os, y::null);
  return output;
}

const y::string* ParseGlobals::lexer_input_contents = y::null;
y::size ParseGlobals::lexer_input_offset = 0;

Node* ParseGlobals::parser_output = y::null;
y::vector<y::string> ParseGlobals::errors;

y::string ParseGlobals::error(
    y::size line, const y::string& token, const y::string& message)
{
  bool replace = false;
  y::string t = token;
  y::size it;
  while ((it = t.find('\n')) != y::string::npos) {
    t.replace(it, 1 + it, "");
    replace = true;
  }
  while ((it = t.find('\r')) != y::string::npos) {
    t.replace(it, 1 + it, "");
    replace = true;
  }
  while ((it = t.find('\t')) != y::string::npos) {
    t.replace(it, 1 + it, " ");
  }
  if (replace) {
    --line;
  }

  y::sstream ss;
  ss << "Error at line " << line <<
      ", near `" << t << "`:\n\t" << message;
  return ss.str();
}
