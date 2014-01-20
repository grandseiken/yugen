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

Program::Program(const Context& context, const y::string& name,
                 const y::string& contents, bool optimise)
  : _context(context)
  , _name(name)
  , _ast(y::null)
  , _module(y::null)
  , _engine(y::null)
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

const Context& Program::get_context() const
{
  return _context;
}

const y::string& Program::get_name() const
{
  return _name;
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

  // The ExecutionEngine takes ownership of the LLVM module (and by extension
  // everything else we created during codegen). So the engine alone must be
  // uniqued and deleted.
  _module = new llvm::Module(_name, llvm::getGlobalContext());
  _engine = y::move_unique(
      llvm::EngineBuilder(_module).setErrorStr(&error).create());
  if (!_engine) {
    log_err("Couldn't create execution engine:\n", error);
    _module = y::null;
  }
  // Disable implicit searching so we don't accidentally resolve linked-in
  // functions.
  _engine->DisableSymbolSearching();

  internal::IrGenerator irgen(*_module, *_engine, _globals);
  irgen.walk(*_ast);
  irgen.emit_global_functions();

  if (llvm::verifyModule(*_module, llvm::ReturnStatusAction, &error)) {
    log_err("Couldn't verify module:\n", error);
    _module = y::null;
  }
  _trampoline_map = irgen.get_trampoline_map();
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
  y::void_fp global_alloc = get_native_fp("!global_alloc");
  typedef void* (*alloc_fp)();
  _global_data = ((alloc_fp)global_alloc)();
}

Instance::~Instance()
{
  y::void_fp global_free = get_native_fp("!global_free");
  typedef void (*free_fp)(void*);
  ((free_fp)global_free)(_global_data);
}

const Program& Instance::get_program() const
{
  return _program;
}

y::void_fp Instance::get_native_fp(const y::string& name) const
{
  return get_native_fp(_program._module->getFunction(name));
}

y::void_fp Instance::get_native_fp(llvm::Function* ir_fp) const
{
  void* void_p = _program._engine->getPointerToFunction(ir_fp);
  // ISO C++ forbids casting between pointer-to-function and pointer-to-object!
  // Unfortunately, due to dependence on dlsym, there doesn't seem to be any
  // way around this (technically) defined behaviour. I guess it should work
  // in practice. Also occurs in irgen.cpp.
  // TODO: maybe it can be resolved somehow?
  return (y::void_fp)(y::intptr)void_p;
}

bool Instance::check_global(const y::string& name, const Type& type,
                            bool for_modification) const
{
  auto it = _program._globals.find(name);
  if (it == _program._globals.end()) {
    log_err(_program._name,
            ": requested global `", name, "` does not exist");
    return false;
  }
  if (type != it->second) {
    log_err(_program._name, ": global `", it->second.string() +
            " ", name, "` accessed via incorrect type `", type.string() + "`");
    return false;
  }
  if (for_modification && (it->second.is_const() || !it->second.is_exported())) {
    log_err(_program._name, ": global `", it->second.string(), " " + name,
            "` cannot be modified externally");
    return false;
  }
  return true;
}

bool Instance::check_function(const y::string& name, const Type& type) const
{
  auto it = _program._functions.find(name);
  if (it == _program._functions.end()) {
    log_err(_program._name,
            ": requested function `", name, "` does not exist");
    return false;
  }
  if (type != it->second) {
    log_err(_program._name, ": function `", it->second.string() +
            " ", name, "` accessed via incorrect type `", type.string(), "`");
    return false;
  }
  return true;
}

// End namespace yang.
}
