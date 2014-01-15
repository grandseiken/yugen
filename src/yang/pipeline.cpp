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
  internal::ParseGlobals::lexer_input_contents = &contents;
  internal::ParseGlobals::lexer_input_offset = 0;
  internal::ParseGlobals::parser_output = y::null;
  internal::ParseGlobals::errors.clear();

  yang_parse();
  y::unique<internal::Node> output =
      y::move_unique(internal::ParseGlobals::parser_output);
  internal::Node::orphans.erase(output.get());
  for (internal::Node* node : internal::Node::orphans) {
    y::move_unique(node);
  }
  internal::Node::orphans.clear();

  for (const y::string& s : internal::ParseGlobals::errors) {
    log_err(s);
  }
  if (internal::ParseGlobals::errors.size()) {
    return;
  }

  internal::StaticChecker checker(_functions, _globals);
  checker.walk(*output);
  if (checker.errors()) {
    _functions.clear();
    _globals.clear();
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
  internal::AstPrinter printer;
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

const Program::symbol_table& Program::get_functions() const
{
  return _functions;
}

const Program::symbol_table& Program::get_globals() const
{
  return _globals;
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

  internal::IrGenerator irgen(*_module, _globals);
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

void* Instance::get_native_fp(const y::string& name) const
{
  llvm::Function* ir_fp = _program._module->getFunction(name);
  return _program._engine->getPointerToFunction(ir_fp);
}

bool Instance::check_global(const y::string& name, const Type& t) const
{
  auto it = _program._globals.find(name);
  if (it == _program._globals.end()) {
    log_err(_program._name +
            ": requested global `" + name + "` does not exist");
    return false;
  }
  if (t != it->second) {
    log_err(_program._name + ": requested global `" + it->second.string() +
            " " + name + "` via incorrect type `" + t.string() + "`");
    return false;
  }
  return true;
}

// End namespace yang.
}
