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

int yang_parse();

namespace yang {

Program::Program(const y::string& name,
                 const y::string& contents, bool optimise)
  : _name(name)
  , _ast(y::null)
  , _module(y::null)
{
  ParseGlobals::lexer_input_contents = &contents;
  ParseGlobals::lexer_input_offset = 0;
  ParseGlobals::parser_output = y::null;
  ParseGlobals::errors.clear();

  yang_parse();
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

  StaticChecker checker(_export_functions, _export_globals, _internal_globals);
  checker.walk(*output);
  if (checker.errors()) {
    _export_functions.clear();
    _export_globals.clear();
    _internal_globals.clear();
    return;
  }
  _ast = y::move_unique(output);

  generate_ir();
  if (optimise) {
    optimise_ir();
  }
}

Program::~Program()
{
}

bool Program::success() const
{
  return bool(_ast) && bool(_module);
}

y::string Program::print_ast() const
{
  if (!success()) {
    return "<error>";
  }
  AstPrinter printer;
  return printer.walk(*_ast) + '\n';
}

y::string Program::print_ir() const
{
  if (!success()) {
    return "<error>";
  }
  y::string output;
  llvm::raw_string_ostream os(output);
  _module->print(os, y::null);
  return output;
}

const Program::symbol_table& Program::get_export_functions() const
{
  return _export_functions;
}

const Program::symbol_table& Program::get_export_globals() const
{
  return _export_globals;
}

const Program::symbol_table& Program::get_internal_globals() const
{
  return _internal_globals;
}

void Program::generate_ir()
{
  y::string error;
  llvm::InitializeNativeTarget();
  _module = y::move_unique(
      new llvm::Module(_name, llvm::getGlobalContext()));

  // TODO: unsure if I am supposed to delete the ExecutionEngine, or the Values.
  _engine = llvm::EngineBuilder(_module.get()).setErrorStr(&error).create();
  if (!_engine) {
    log_err("Couldn't create execution engine:\n", error);
    _module = y::null;
  }

  symbol_table all_globals;
  for (const auto& pair : _export_globals) {
    all_globals.insert(pair);
  }
  for (const auto& pair : _internal_globals) {
    all_globals.insert(pair);
  }

  IrGenerator irgen(*_module, all_globals);
  irgen.walk(*_ast);
  irgen.emit_global_functions();

  if (llvm::verifyModule(*_module, llvm::ReturnStatusAction, &error)) {
    log_err("Couldn't verify module:\n", error);
    _module = y::null;
  }
}

void Program::optimise_ir()
{
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

Instance::Instance(const Program& program)
  : _program(program)
  , _global_data(y::null)
{
  void* global_alloc = get_native_fp("!global_alloc");
  _global_data = ((void* (*)())global_alloc)();
}

Instance::~Instance()
{
  void* global_free = get_native_fp("!global_free");
  ((void (*)(void*))global_free)(_global_data);
}

void* Instance::get_native_fp(const y::string& name)
{
  llvm::Function* ir_fp = _program._module->getFunction(name);
  return _program._engine->getPointerToFunction(ir_fp);
}

// End namespace yang.
}
