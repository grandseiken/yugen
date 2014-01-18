#include "irgen.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/Module.h>
#include "../log.h"

namespace llvm {
  class LLVMContext;
}

namespace std {
  template<>
  struct hash<yang::internal::IrGenerator::metadata> {
    y::size operator()(yang::internal::IrGenerator::metadata v) const
    {
      return v;
    }
  };
}

namespace yang {
namespace internal {

IrGeneratorUnion::IrGeneratorUnion(llvm::Type* type)
  : type(type)
  , value(y::null)
{
}

IrGeneratorUnion::IrGeneratorUnion(llvm::Value* value)
  : type(y::null)
  , value(value)
{
}

IrGeneratorUnion::operator llvm::Type*() const
{
  return type;
}

IrGeneratorUnion::operator llvm::Value*() const
{
  return value;
}

IrGenerator::IrGenerator(llvm::Module& module, llvm::ExecutionEngine& engine,
                         symbol_frame& globals)
  : _module(module)
  , _engine(engine)
  , _builder(module.getContext())
  , _symbol_table(y::null)
  , _metadata(y::null)
{
  // Set up the global data type. Since each program is designed to support
  // multiple independent instances running simultaeneously, the idea is to
  // define a structure type with a field for each global variable. Each
  // function will take a pointer to the global data structure as an implicit
  // first parameter.
  y::vector<llvm::Type*> type_list;
  y::size number = 0;
  // Since the global data structure may contain pointers to functions which
  // themselves take said implicit argument, we need to use an opaque named
  // struct create the potentially-recursive type.
  auto global_struct = 
      llvm::StructType::create(module.getContext(), "global_data");
  _global_data = llvm::PointerType::get(global_struct, 0);
  for (const auto& pair : globals) {
    // Type-calculation must be kept up-to-date with new types, or globals of
    // that type will fail.
    llvm::Type* t = get_llvm_type(pair.second);
    type_list.push_back(t);
    _global_numbering[pair.first] = number++;
  }
  // Structures can't be empty, so add a dummy member (it's never used).
  if (type_list.empty()) {
    type_list.push_back(int_type());
  }
  global_struct->setBody(type_list, false);
}

IrGenerator::~IrGenerator()
{
  // Keep hash<metadata> in source file.
}

void IrGenerator::emit_global_functions()
{
  auto& b = _builder;

  // Register malloc and free. Malloc takes a pointer argument, since
  // we computer sizeof with pointer arithmetic. (Conveniently, it's
  // also a type of the correct bit-width.)
  auto malloc_ptr = get_native_function(
      "malloc", (void_fp)&malloc,
      llvm::FunctionType::get(_global_data, _global_data, false));
  auto free_ptr = get_native_function(
      "free", (void_fp)&free,
      llvm::FunctionType::get(void_type(), _global_data, false));

  // Create allocator function.
  auto alloc = llvm::Function::Create(
      llvm::FunctionType::get(_global_data, false),
      llvm::Function::ExternalLinkage, "!global_alloc", &_module);
  auto alloc_block = llvm::BasicBlock::Create(
      b.getContext(), "entry", alloc);
  b.SetInsertPoint(alloc_block);

  // Compute sizeof(_global_data) by indexing one past the null pointer.
  llvm::Value* size_of = b.CreateIntToPtr(
      constant_int(0), _global_data, "null");
  size_of = b.CreateGEP(
      size_of, constant_int(1), "sizeof");
  // Call malloc, call each of the global initialisation functions, and return
  // the pointer.
  llvm::Value* v = b.CreateCall(malloc_ptr, size_of, "call");
  for (llvm::Function* f : _global_inits) {
    b.CreateCall(f, v);
  }
  b.CreateRet(v);

  // Create free function.
  auto free = llvm::Function::Create(
      llvm::FunctionType::get(void_type(), _global_data, false),
      llvm::Function::ExternalLinkage, "!global_free", &_module);
  auto free_block = llvm::BasicBlock::Create(
      b.getContext(), "entry", free);
  b.SetInsertPoint(free_block);
  auto it = free->arg_begin();
  it->setName("global");
  b.CreateCall(free_ptr, it);
  b.CreateRetVoid();

  // Create accessor functions for each field of the global structure.
  for (const auto pair : _global_numbering) {
    llvm::Type* t = _global_data->
        getPointerElementType()->getStructElementType(pair.second);

    y::string name = "!global_get_" + pair.first;
    auto function_type = llvm::FunctionType::get(t, _global_data, false);
    auto getter = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, name, &_module);
    _trampoline_map.emplace(name, create_trampoline_function(function_type));

    auto getter_block = llvm::BasicBlock::Create(
        b.getContext(), "entry", getter);
    auto it = getter->arg_begin();
    it->setName("global");
    b.SetInsertPoint(getter_block);
    b.CreateRet(
        b.CreateLoad(global_ptr(it, pair.second), "load"));

    name = "!global_set_" + pair.first;
    y::vector<llvm::Type*> setter_args{_global_data, t};
    function_type = llvm::FunctionType::get(void_type(), setter_args, false);
    auto setter = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, name, &_module);
    _trampoline_map.emplace(name, create_trampoline_function(function_type));

    auto setter_block = llvm::BasicBlock::Create(
        b.getContext(), "entry", setter);
    it = setter->arg_begin();
    it->setName("global");
    auto jt = it;
    ++jt;
    jt->setName("value");
    b.SetInsertPoint(setter_block);
    b.CreateStore(jt, global_ptr(it, pair.second));
    b.CreateRetVoid();
  }
}

const IrGenerator::trampoline_map& IrGenerator::get_trampoline_map() const
{
  return _trampoline_map;
}

void IrGenerator::preorder(const Node& node)
{
  auto& b = _builder;

  switch (node.type) {
    case Node::GLOBAL:
    {
      _metadata.push();
      // GLOBAL init functions don't need external linkage, since they are
      // called automatically by the externally-visible global structure
      // allocation function.
      auto function = llvm::Function::Create(
          llvm::FunctionType::get(void_type(), _global_data, false),
          llvm::Function::InternalLinkage, "!global_init", &_module);
      _global_inits.push_back(function);
      auto block = llvm::BasicBlock::Create(b.getContext(), "entry", function);

      _metadata.add(GLOBAL_INIT_FUNCTION, function);
      b.SetInsertPoint(block);
      _symbol_table.push();

      // Store a special entry in the symbol table for the implicit global
      // structure pointer.
      auto it = function->arg_begin();
      it->setName("global");
      _metadata.add(GLOBAL_DATA_PTR, it);
      _metadata.add(FUNCTION, function);
      break;
    }

    case Node::GLOBAL_ASSIGN:
    case Node::ASSIGN_VAR:
    case Node::ASSIGN_CONST:
      // See static.cpp for details.
      if (node.children[0]->type == Node::FUNCTION) {
        _immediate_left_assign = node.string_value;
      }
      break;

    case Node::BLOCK:
      _symbol_table.push();
      break;

    case Node::IF_STMT:
    {
      _metadata.push();
      _symbol_table.push();
      create_block(IF_THEN_BLOCK, "then");
      create_block(MERGE_BLOCK, "merge");
      if (node.children.size() > 2) {
        create_block(IF_ELSE_BLOCK, "else");
      }
      break;
    }

    case Node::FOR_STMT:
    {
      _metadata.push();
      _symbol_table.push();
      create_block(LOOP_COND_BLOCK, "cond");
      create_block(LOOP_BODY_BLOCK, "loop");
      auto after_block = create_block(LOOP_AFTER_BLOCK, "after");
      auto merge_block = create_block(MERGE_BLOCK, "merge");
      _metadata.add(LOOP_BREAK_LABEL, merge_block);
      _metadata.add(LOOP_CONTINUE_LABEL, after_block);
      break;
    }

    case Node::DO_WHILE_STMT:
    {
      _metadata.push();
      _symbol_table.push();
      auto loop_block =create_block(LOOP_BODY_BLOCK, "loop");
      auto cond_block = create_block(LOOP_COND_BLOCK, "cond");
      auto merge_block = create_block(MERGE_BLOCK, "merge");
      _metadata.add(LOOP_BREAK_LABEL, merge_block);
      _metadata.add(LOOP_CONTINUE_LABEL, cond_block);

      b.CreateBr(loop_block);
      b.SetInsertPoint(loop_block);
      break;
    }

    default: {}
  }
}

void IrGenerator::infix(const Node& node, const result_list& results)
{
  auto& b = _builder;

  switch (node.type) {
    case Node::FUNCTION:
    {
      create_function(node, (llvm::FunctionType*)results[0].type);
      break;
    }

    case Node::IF_STMT:
    {
      auto then_block = (llvm::BasicBlock*)_metadata[IF_THEN_BLOCK];
      auto else_block = (llvm::BasicBlock*)_metadata[IF_ELSE_BLOCK];
      auto merge_block = (llvm::BasicBlock*)_metadata[MERGE_BLOCK];

      if (results.size() == 1) {
        bool has_else = node.children.size() > 2;
        b.CreateCondBr(i2b(results[0]), then_block,
                       has_else ? else_block : merge_block);
        b.SetInsertPoint(then_block);
      }
      if (results.size() == 2) {
        b.CreateBr(merge_block);
        b.SetInsertPoint(else_block);
      }
      break;
    }

    case Node::FOR_STMT:
    {
      auto cond_block = (llvm::BasicBlock*)_metadata[LOOP_COND_BLOCK];
      auto loop_block = (llvm::BasicBlock*)_metadata[LOOP_BODY_BLOCK];
      auto after_block = (llvm::BasicBlock*)_metadata[LOOP_AFTER_BLOCK];
      auto merge_block = (llvm::BasicBlock*)_metadata[MERGE_BLOCK];

      if (results.size() == 1) {
        b.CreateBr(cond_block);
        b.SetInsertPoint(cond_block);
      }
      if (results.size() == 2) {
        b.CreateCondBr(i2b(results[1]), loop_block, merge_block);
        b.SetInsertPoint(after_block);
      }
      if (results.size() == 3) {
        b.CreateBr(cond_block);
        b.SetInsertPoint(loop_block);
      }
      break;
    }

    case Node::DO_WHILE_STMT:
    {
      auto cond_block = (llvm::BasicBlock*)_metadata[LOOP_COND_BLOCK];
      b.CreateBr(cond_block);
      b.SetInsertPoint(cond_block);
      break;
    }

    case Node::TERNARY:
    {
      // Vectorised ternary can't short-circuit.
      if (results[0].value->getType()->isVectorTy()) {
        break;
      }

      if (results.size() == 1) {
        _metadata.push();

        // Blocks and branching are necessary (as opposed to a select instruction)
        // to short-circuit and avoiding evaluating the other path.
        auto then_block = create_block(IF_THEN_BLOCK, "then");
        auto else_block = create_block(IF_ELSE_BLOCK, "else");
        create_block(MERGE_BLOCK, "merge");

        b.CreateCondBr(i2b(results[0]), then_block, else_block);
        b.SetInsertPoint(then_block);
      }
      if (results.size() == 2) {
        auto else_block = (llvm::BasicBlock*)_metadata[IF_ELSE_BLOCK];
        auto merge_block = (llvm::BasicBlock*)_metadata[MERGE_BLOCK];
        b.CreateBr(merge_block);
        b.SetInsertPoint(else_block);
      }
      break;
    }

    // Logical OR and AND short-circuit. This interacts in a subtle way with
    // vectorisation: we can short-circuit only when the left-hand operand is a
    // primitive. (We could also check and short-circuit if the left-hand
    // operand is an entirely zero or entirely non-zero vector. We currently
    // don't; it seems a bit daft.)
    case Node::LOGICAL_OR:
    case Node::LOGICAL_AND:
    {
      if (results[0].value->getType()->isVectorTy()) {
        break;
      }
      _metadata.push();
      _metadata.add(LOGICAL_OP_SOURCE_BLOCK, b.GetInsertPoint());
      auto rhs_block = create_block(LOGICAL_OP_RHS_BLOCK, "rhs");
      auto merge_block = create_block(MERGE_BLOCK, "merge");

      if (node.type == Node::LOGICAL_OR) {
        b.CreateCondBr(i2b(results[0]), merge_block, rhs_block);
      }
      else {
        b.CreateCondBr(i2b(results[0]), rhs_block, merge_block);
      }
      b.SetInsertPoint(rhs_block);
      break;
    }

    default: {}
  }
}

IrGeneratorUnion IrGenerator::visit(const Node& node,
                                    const result_list& results)
{
  auto& b = _builder;
  auto parent = b.GetInsertBlock() ? b.GetInsertBlock()->getParent() : y::null;
  y::vector<llvm::Type*> types;
  for (const auto& v : results) {
    types.push_back(v.value ? v.value->getType() : y::null);
  }

  auto binary_lambda = [&](llvm::Value* v, llvm::Value* u)
  {
    Node::node_type type =
        node.type == Node::FOLD_LOGICAL_OR ? Node::LOGICAL_OR :
        node.type == Node::FOLD_LOGICAL_AND ? Node::LOGICAL_AND :
        node.type == Node::FOLD_BITWISE_OR ? Node::BITWISE_OR :
        node.type == Node::FOLD_BITWISE_AND ? Node::BITWISE_AND :
        node.type == Node::FOLD_BITWISE_XOR ? Node::BITWISE_XOR :
        node.type == Node::FOLD_BITWISE_LSHIFT ? Node::BITWISE_LSHIFT :
        node.type == Node::FOLD_BITWISE_RSHIFT ? Node::BITWISE_RSHIFT :
        node.type == Node::FOLD_POW ? Node::POW :
        node.type == Node::FOLD_MOD ? Node::MOD :
        node.type == Node::FOLD_ADD ? Node::ADD :
        node.type == Node::FOLD_SUB ? Node::SUB :
        node.type == Node::FOLD_MUL ? Node::MUL :
        node.type == Node::FOLD_DIV ? Node::DIV :
        node.type == Node::FOLD_EQ ? Node::EQ :
        node.type == Node::FOLD_NE ? Node::NE :
        node.type == Node::FOLD_GE ? Node::GE :
        node.type == Node::FOLD_LE ? Node::LE :
        node.type == Node::FOLD_GT ? Node::GT :
        node.type == Node::FOLD_LT ? Node::LT :
        node.type;

    if (v->getType()->isIntOrIntVectorTy()) {
      return type == Node::LOGICAL_OR ? b.CreateOr(v, u, "lor") :
             type == Node::LOGICAL_AND ? b.CreateAnd(v, u, "land") :
             type == Node::BITWISE_OR ? b.CreateOr(v, u, "or") :
             type == Node::BITWISE_AND ? b.CreateAnd(v, u, "and") :
             type == Node::BITWISE_XOR ? b.CreateXor(v, u, "xor") :
             type == Node::BITWISE_LSHIFT ? b.CreateShl(v, u, "lsh") :
             type == Node::BITWISE_RSHIFT ? b.CreateAShr(v, u, "rsh") :
             type == Node::POW ? pow(v, u) :
             type == Node::MOD ? mod(v, u) :
             type == Node::ADD ? b.CreateAdd(v, u, "add") :
             type == Node::SUB ? b.CreateSub(v, u, "sub") :
             type == Node::MUL ? b.CreateMul(v, u, "mul") :
             type == Node::DIV ? div(v, u) :
             type == Node::EQ ? b.CreateICmpEQ(v, u, "eq") :
             type == Node::NE ? b.CreateICmpNE(v, u, "ne") :
             type == Node::GE ? b.CreateICmpSGE(v, u, "ge") :
             type == Node::LE ? b.CreateICmpSLE(v, u, "le") :
             type == Node::GT ? b.CreateICmpSGT(v, u, "gt") :
             type == Node::LT ? b.CreateICmpSLT(v, u, "lt") :
             y::null;
    }
    return type == Node::POW ? pow(v, u) :
           type == Node::MOD ? mod(v, u) :
           type == Node::ADD ? b.CreateFAdd(v, u, "fadd") :
           type == Node::SUB ? b.CreateFSub(v, u, "fsub") :
           type == Node::MUL ? b.CreateFMul(v, u, "fmul") :
           type == Node::DIV ? div(v, u) :
           type == Node::EQ ? b.CreateFCmpOEQ(v, u, "feq") :
           type == Node::NE ? b.CreateFCmpONE(v, u, "fne") :
           type == Node::GE ? b.CreateFCmpOGE(v, u, "fge") :
           type == Node::LE ? b.CreateFCmpOLE(v, u, "fle") :
           type == Node::GT ? b.CreateFCmpOGT(v, u, "fgt") :
           type == Node::LT ? b.CreateFCmpOLT(v, u, "flt") :
           y::null;
  };

  switch (node.type) {
    case Node::TYPE_VOID:
      return void_type();
    case Node::TYPE_INT:
      return node.int_value > 1 ?
          vector_type(int_type(), node.int_value) : int_type();
    case Node::TYPE_WORLD:
      return node.int_value > 1 ?
          vector_type(world_type(), node.int_value) : world_type();
    case Node::TYPE_FUNCTION:
    {
      // When passing functions as arguments and returning them from functions,
      // we need to take use the pointer-type, as the bare type is not
      // equivalent (and is not first-class in LLVM).
      auto fn_ptr = [](llvm::Type* t)
      {
        return t->isFunctionTy() ? llvm::PointerType::get(t, 0) : t;
      };

      y::vector<llvm::Type*> args;
      args.push_back(_global_data);
      for (y::size i = 1; i < results.size(); ++i) {
        args.push_back(fn_ptr(results[i]));
      }
      return llvm::FunctionType::get(fn_ptr(results[0]), args, false);
    }

    case Node::GLOBAL:
      b.CreateRetVoid();
      _symbol_table.pop();
      _metadata.pop();
      return results[0];
    case Node::GLOBAL_ASSIGN:
    {
      auto function = (llvm::Function*)parent;
      auto function_type =
          (llvm::FunctionType*)function->getType()->getPointerElementType();
      _trampoline_map.emplace(
          node.string_value, create_trampoline_function(function_type));
      // Top-level functions Nodes have their int_value set to 1 when defined
      // using the `export` keyword.
      if (node.int_value) {
        function->setLinkage(llvm::Function::ExternalLinkage);
      }
      function->setName(node.string_value);
      _symbol_table.add(node.string_value, results[0]);
      return results[0];
    }
    case Node::FUNCTION:
    {
      auto function_type =
          (llvm::FunctionType*)parent->getType()->getElementType();
      if (function_type->getReturnType()->isVoidTy()) {
        b.CreateRetVoid();
      }
      else {
        // In a function that passes the static check, control never reaches
        // this point; but the block must have a terminator.
        b.CreateBr(_builder.GetInsertBlock());
      }
      _symbol_table.pop();
      _symbol_table.pop();
      _metadata.pop();
      // If this was a nested function, set the insert point back to the last
      // block in the enclosing function.
      if (_metadata.has(FUNCTION)) {
        auto function = (llvm::Function*)_metadata[FUNCTION];
        b.SetInsertPoint(&*function->getBasicBlockList().rbegin());
      }
      return parent;
    }

    case Node::BLOCK:
    {
      auto after_block =
          llvm::BasicBlock::Create(b.getContext(), "after", parent);
      b.CreateBr(after_block);
      b.SetInsertPoint(after_block);
      _symbol_table.pop();
      return parent;
    }
    case Node::EMPTY_STMT:
      return constant_int(0);
    case Node::EXPR_STMT:
      return results[0];
    case Node::RETURN_STMT:
    {
      auto dead_block =
          llvm::BasicBlock::Create(b.getContext(), "dead", parent);
      llvm::Value* v = b.CreateRet(results[0]);
      b.SetInsertPoint(dead_block);
      return v;
    }
    case Node::IF_STMT:
    {
      auto merge_block = (llvm::BasicBlock*)_metadata[MERGE_BLOCK];
      _symbol_table.pop();
      _metadata.pop();
      b.CreateBr(merge_block);
      b.SetInsertPoint(merge_block);
      return results[0];
    }
    case Node::FOR_STMT:
    {
      auto after_block = (llvm::BasicBlock*)_metadata[LOOP_AFTER_BLOCK];
      auto merge_block = (llvm::BasicBlock*)_metadata[MERGE_BLOCK];
      _symbol_table.pop();
      _metadata.pop();

      b.CreateBr(after_block);
      b.SetInsertPoint(merge_block);
      return results[0];
    }
    case Node::DO_WHILE_STMT:
    {
      auto loop_block = (llvm::BasicBlock*)_metadata[LOOP_BODY_BLOCK];
      auto merge_block = (llvm::BasicBlock*)_metadata[MERGE_BLOCK];
      _symbol_table.pop();
      _metadata.pop();

      b.CreateCondBr(i2b(results[1]), loop_block, merge_block);
      b.SetInsertPoint(merge_block);
      return results[0];
    }
    case Node::BREAK_STMT:
    case Node::CONTINUE_STMT:
    {
      auto dead_block =
          llvm::BasicBlock::Create(b.getContext(), "dead", parent);
      llvm::Value* v = b.CreateBr((llvm::BasicBlock*)_metadata[
          node.type == Node::BREAK_STMT ? LOOP_BREAK_LABEL :
                                          LOOP_CONTINUE_LABEL]);
      b.SetInsertPoint(dead_block);
      return v;
    }

    case Node::IDENTIFIER:
      // Load the local variable, if it's there.
      if (_symbol_table.has(node.string_value) &&
          _symbol_table.index(node.string_value)) {
        return b.CreateLoad(_symbol_table[node.string_value], "load");
      }
      // If it's a function (constant global), just get the value.
      if (!_symbol_table.index(node.string_value)) {
        return _symbol_table.get(node.string_value, 0);
      }
      // Otherwise it's a global, so look up in the global structure.
      return b.CreateLoad(global_ptr(node.string_value), "load");

    case Node::INT_LITERAL:
      return constant_int(node.int_value);
    case Node::WORLD_LITERAL:
      return constant_world(node.world_value);

    case Node::TERNARY:
    {
      if (results[0].value->getType()->isVectorTy()) {
        // Vectorised ternary. Short-circuiting isn't possible.
        return b.CreateSelect(i2b(results[0]), results[1], results[2]);
      }
      auto then_block = (llvm::BasicBlock*)_metadata[IF_THEN_BLOCK];
      auto else_block = (llvm::BasicBlock*)_metadata[IF_ELSE_BLOCK];
      auto merge_block = (llvm::BasicBlock*)_metadata[MERGE_BLOCK];
      _metadata.pop();
      b.CreateBr(merge_block);
      b.SetInsertPoint(merge_block);
      auto phi = b.CreatePHI(types[1], 2, "tern");
      phi->addIncoming(results[1], then_block);
      phi->addIncoming(results[2], else_block);
      return phi;
    }
    case Node::CALL:
    {
      y::vector<llvm::Value*> args;
      args.push_back(_metadata[GLOBAL_DATA_PTR]);
      for (y::size i = 1; i < results.size(); ++i) {
        args.push_back(results[i]);
      }
      // No name, since it may have type VOID.
      return b.CreateCall(results[0], args);
    }

    case Node::LOGICAL_OR:
    case Node::LOGICAL_AND:
    {
      if (types[0]->isVectorTy()) {
        // Short-circuiting isn't possible.
        return b2i(binary(i2b(results[0]), i2b(results[1]), binary_lambda));
      }
      auto source_block = (llvm::BasicBlock*)_metadata[LOGICAL_OP_SOURCE_BLOCK];
      auto rhs_block = (llvm::BasicBlock*)_metadata[LOGICAL_OP_RHS_BLOCK];
      auto merge_block = (llvm::BasicBlock*)_metadata[MERGE_BLOCK];
      _metadata.pop();

      auto rhs = b2i(i2b(results[1]));
      b.CreateBr(merge_block);
      b.SetInsertPoint(merge_block);
      llvm::Constant* constant =
          constant_int(node.type == Node::LOGICAL_OR ? 1 : 0);
      llvm::Type* type = int_type();
      if (types[1]->isVectorTy()) {
        y::size n = types[1]->getVectorNumElements();
        constant = constant_vector(constant, n);
        type = vector_type(type, n);
      }

      auto phi = b.CreatePHI(type, 2, "cut");
      phi->addIncoming(constant, source_block);
      phi->addIncoming(rhs, rhs_block);
      return phi;
    }

    // Most binary operators map directly to (vectorisations of) LLVM IR
    // instructions.
    case Node::BITWISE_OR:
    case Node::BITWISE_AND:
    case Node::BITWISE_XOR:
    case Node::BITWISE_LSHIFT:
    case Node::BITWISE_RSHIFT:
      return binary(results[0], results[1], binary_lambda);

    case Node::POW:
    case Node::MOD:
    case Node::ADD:
    case Node::SUB:
    case Node::MUL:
    case Node::DIV:
      return binary(results[0], results[1], binary_lambda);
    case Node::EQ:
    case Node::NE:
    case Node::GE:
    case Node::LE:
    case Node::GT:
    case Node::LT:
      return b2i(binary(results[0], results[1], binary_lambda));

    case Node::FOLD_LOGICAL_OR:
    case Node::FOLD_LOGICAL_AND:
      return b2i(fold(results[0], binary_lambda, true));

    case Node::FOLD_BITWISE_OR:
    case Node::FOLD_BITWISE_AND:
    case Node::FOLD_BITWISE_XOR:
    case Node::FOLD_BITWISE_LSHIFT:
    case Node::FOLD_BITWISE_RSHIFT:
      return fold(results[0], binary_lambda);

    case Node::FOLD_POW:
      // POW is the only right-associative fold operator.
      return fold(results[0], binary_lambda, false, false, true);
    case Node::FOLD_MOD:
    case Node::FOLD_ADD:
    case Node::FOLD_SUB:
    case Node::FOLD_MUL:
    case Node::FOLD_DIV:
      return fold(results[0], binary_lambda);

    case Node::FOLD_EQ:
    case Node::FOLD_NE:
    case Node::FOLD_GE:
    case Node::FOLD_LE:
    case Node::FOLD_GT:
    case Node::FOLD_LT:
      return b2i(fold(results[0], binary_lambda, false, true));

    case Node::LOGICAL_NEGATION:
    {
      llvm::Constant* cmp = constant_int(0);
      if (types[0]->isVectorTy()) {
        cmp = constant_vector(cmp, types[0]->getVectorNumElements());
      }
      return b2i(b.CreateICmpEQ(results[0], cmp, "lneg"));
    }
    case Node::BITWISE_NEGATION:
    {
      llvm::Constant* cmp = constant_int((y::uint32)(0) - 1);
      if (types[0]->isVectorTy()) {
        cmp = constant_vector(cmp, types[0]->getVectorNumElements());
      }
      return b.CreateXor(results[0], cmp, "neg");
    }
    case Node::ARITHMETIC_NEGATION:
    {
      llvm::Constant* cmp = types[0]->isIntOrIntVectorTy() ?
          constant_int(0) : constant_world(0);
      if (types[0]->isVectorTy()) {
        cmp = constant_vector(cmp, types[0]->getVectorNumElements());
      }
      return types[0]->isIntOrIntVectorTy() ?
          b.CreateSub(cmp, results[0], "sub") :
          b.CreateFSub(cmp, results[0], "fsub");
    }

    case Node::ASSIGN:
      // See Node::IDENTIFIER.
      if (_symbol_table.has(node.string_value)) {
        b.CreateStore(results[0], _symbol_table[node.string_value]);
      }
      // We can't store values for globals in the symbol table since the lookup
      // depends on the current function's global data argument.
      else {
        b.CreateStore(results[0], global_ptr(node.string_value));
      }
      return results[0];

    case Node::ASSIGN_VAR:
    case Node::ASSIGN_CONST:
    {
      // In a global block, rather than allocating anything we simply store into
      // the prepared fields of the global structure.
      if (_metadata.has(GLOBAL_INIT_FUNCTION)) {
        b.CreateStore(results[0], global_ptr(node.string_value));
        return results[0];
      }

      // Optimisation passes such as mem2reg work much better when memory
      // locations are declared in the entry block (so they are guaranteed to
      // execute once).
      auto& entry_block =
          ((llvm::Function*)b.GetInsertPoint()->getParent())->getEntryBlock();
      llvm::IRBuilder<> entry(&entry_block, entry_block.begin());
      llvm::Value* v = entry.CreateAlloca(types[0], y::null, node.string_value);
      b.CreateStore(results[0], v);
      _symbol_table.add(node.string_value, v);
      return results[0];
    }

    case Node::INT_CAST:
      return w2i(results[0]);
    case Node::WORLD_CAST:
      return i2w(results[0]);

    case Node::VECTOR_CONSTRUCT:
    {
      llvm::Value* v = constant_vector(
          types[0]->isIntegerTy() ?
              constant_int(0) : constant_world(0), results.size());
      for (y::size i = 0; i < results.size(); ++i) {
        v = b.CreateInsertElement(v, results[i], constant_int(i), "vec");
      }
      return v;
    }
    case Node::VECTOR_INDEX:
    {
      // Indexing out-of-bounds produces constant zero.
      llvm::Constant* zero = types[0]->isIntOrIntVectorTy() ?
          constant_int(0) : constant_world(0);
      auto ge = b.CreateICmpSGE(results[1], constant_int(0), "idx");
      auto lt = b.CreateICmpSLT(
          results[1], constant_int(types[0]->getVectorNumElements()), "idx");

      auto in = b.CreateAnd(ge, lt, "idx");
      return b.CreateSelect(
          in, b.CreateExtractElement(results[0], results[1], "idx"),
          zero, "idx");
    }

    default:
      return (llvm::Value*)y::null;
  }
}

llvm::Function* IrGenerator::get_native_function(
    const y::string& name, void_fp native_fp, llvm::FunctionType* type) const
{
  // We use special !-prefixed names for native functions so that they can't
  // be confused with regular user-defined functions (e.g. "malloc" is not
  // reserved).
  llvm::Function* llvm_function = llvm::Function::Create(
      type, llvm::Function::ExternalLinkage, "!" + name, &_module);
  // We need to explicitly link the LLVM function to the native function.
  // More (technically) undefined behaviour here.
  _engine.addGlobalMapping(llvm_function, (void*)(y::intptr)native_fp);
  return llvm_function;
}

void IrGenerator::create_function(
    const Node& node, llvm::FunctionType* function_type)
{
  auto& b = _builder;
  _metadata.push();

  // Linkage will be set later if necessary.
  auto function = llvm::Function::Create(
      function_type, llvm::Function::InternalLinkage,
      "anonymous", &_module);

  auto block = llvm::BasicBlock::Create(b.getContext(), "entry", function);
  b.SetInsertPoint(block);

  _symbol_table.push();
  // Recursive lookup handled similarly to arguments below.
  if (_immediate_left_assign.length()) {
    llvm::Value* v = b.CreateAlloca(
        llvm::PointerType::get(function_type, 0), y::null,
        _immediate_left_assign);
    b.CreateStore(function, v);
    _symbol_table.add(_immediate_left_assign, v);
    _immediate_left_assign.clear();
  }
  _symbol_table.push();

  // The code for Node::TYPE_FUNCTION in visit() ensures it takes a global
  // data structure pointer.
  auto it = function->arg_begin();
  it->setName("global");
  _metadata.add(GLOBAL_DATA_PTR, it);
  // Set up the arguments.
  y::size arg_num = 0;
  for (++it; it != function->arg_end(); ++it) {
    const y::string& name =
        node.children[0]->children[1 + arg_num]->string_value;

    // Rather than reference argument values directly, we create an alloca
    // and store the argument in there. This simplifies things, since we
    // can emit the same IR code when referencing local variables or
    // function arguments.
    llvm::Value* alloc = b.CreateAlloca(
        function_type->getParamType(1 + arg_num), y::null, name);
    b.CreateStore(it, alloc);
    _symbol_table.add(name, alloc);
    ++arg_num;
  }
  _metadata.add(FUNCTION, function);
}

llvm::Function* IrGenerator::create_trampoline_function(
    llvm::FunctionType* function_type)
{
  // LLVM types are uniqued, so using them directly as keys for the trampoline
  // cache makes sense.
  auto it = _trampoline_functions.find(function_type);
  if (it != _trampoline_functions.end()) {
    return it->second;  
  }

  auto& b = _builder;
  // LLVM bytecode calling convention has some significant drawbacks that make
  // it unsuitable for direct interop with native code. Most importantly,
  // calling convention for vectors in undefined; vectors can't be passed
  // directly at all. More subtly, calling convention for value structures
  // may be target-dependent.
  //
  // It's possible this implementation is overly conservative, but it's
  // definitely going to work. Essentially, we unpack vectors into individual
  // values, and convert return values to pointer arguments; the trampoline
  // takes the actual function to be called as the final argument.
  y::vector<llvm::Type*> args;
  auto add_type = [&](llvm::Type* t, bool to_ptr)
  {
    if (!t->isVectorTy()) {
      args.push_back(to_ptr ? llvm::PointerType::get(t, 0) : t);
      return;
    }
    for (y::size i = 0; i < t->getVectorNumElements(); ++i) {
      auto elem = t->getVectorElementType();
      args.push_back(to_ptr ? llvm::PointerType::get(elem, 0) : elem);
    }
  };

  auto return_type = function_type->getReturnType();
  y::size return_args =
      return_type->isVoidTy() ? 0 :
      return_type->isVectorTy() ? return_type->getVectorNumElements() : 1;

  // Construct the type of the trampoline function.
  if (!return_type->isVoidTy()) {
    add_type(return_type, true);
  }
  for (auto it = function_type->param_begin();
       it != function_type->param_end(); ++it) {
    add_type(*it, false);
  }
  args.push_back(llvm::PointerType::get(function_type, 0));
  auto ext_function_type = llvm::FunctionType::get(void_type(), args, false);

  // Generate the function code.
  auto function = llvm::Function::Create(
      ext_function_type, llvm::Function::ExternalLinkage,
      "trampoline", &_module);
  auto block = llvm::BasicBlock::Create(b.getContext(), "entry", function);
  b.SetInsertPoint(block);

  y::vector<llvm::Value*> call_args;
  auto callee = --function->arg_end();

  // Translate trampoline arguments to an LLVM-internal argument list.
  auto jt = function->arg_begin();
  for (y::size i = 0; i < return_args; ++i) {
    ++jt;
  }
  for (auto it = function_type->param_begin();
       it != function_type->param_end(); ++it) {
    if (!(*it)->isVectorTy()) {
      call_args.push_back(jt++);
      continue;
    }
    y::size size = (*it)->getVectorNumElements();
    llvm::Value* v = (*it)->isIntOrIntVectorTy() ?
        constant_vector(constant_int(0), size) :
        constant_vector(constant_world(0), size);

    for (y::size i = 0; i < size; ++i) {
      v = b.CreateInsertElement(v, jt, constant_int(i), "vec");
      ++jt;
    }
    call_args.push_back(v);
  }

  // Do the call and translate the result back to native calling convention.
  llvm::Value* result = b.CreateCall(callee, call_args);
  if (!return_type->isVectorTy()) {
    if (!return_type->isVoidTy()) {
      b.CreateStore(result, function->arg_begin());
    }
  }
  else {
    auto it = function->arg_begin();
    for (y::size i = 0; i < return_type->getVectorNumElements(); ++i) {
      llvm::Value* v = b.CreateExtractElement(result, constant_int(i), "vec");
      b.CreateStore(v, it++);
    }
  }
  b.CreateRetVoid();
  _trampoline_functions.emplace(function_type, function);
  return function;
}

llvm::Type* IrGenerator::void_type() const
{
  return llvm::Type::getVoidTy(_builder.getContext());
}

llvm::Type* IrGenerator::int_type() const
{
  return llvm::IntegerType::get(
      _builder.getContext(), 8 * sizeof(Node::int_value));
}

llvm::Type* IrGenerator::world_type() const
{
  return llvm::Type::getDoubleTy(_builder.getContext());
}

llvm::Type* IrGenerator::vector_type(llvm::Type* type, y::size n) const
{
  return llvm::VectorType::get(type, n);
}

llvm::Constant* IrGenerator::constant_int(y::int32 value) const
{
  return llvm::ConstantInt::getSigned(int_type(), value);
}

llvm::Constant* IrGenerator::constant_world(y::world value) const
{
  return llvm::ConstantFP::get(_builder.getContext(), llvm::APFloat(value));
}

llvm::Constant* IrGenerator::constant_vector(
    const y::vector<llvm::Constant*>& values) const
{
  return llvm::ConstantVector::get(values);
}

llvm::Constant* IrGenerator::constant_vector(
    llvm::Constant* value, y::size n) const
{
  return llvm::ConstantVector::getSplat(n, value);
}

llvm::Value* IrGenerator::i2b(llvm::Value* v)
{
  llvm::Constant* cmp = constant_int(0);
  if (v->getType()->isVectorTy()) {
    cmp = constant_vector(cmp, v->getType()->getVectorNumElements());
  }
  return _builder.CreateICmpNE(v, cmp, "bool");
}

llvm::Value* IrGenerator::b2i(llvm::Value* v)
{
  llvm::Type* type = int_type();
  if (v->getType()->isVectorTy()) {
    type = vector_type(type, v->getType()->getVectorNumElements());
  }
  return _builder.CreateZExt(v, type, "int");
}

llvm::Value* IrGenerator::i2w(llvm::Value* v)
{
  llvm::Type* type = world_type();
  if (v->getType()->isVectorTy()) {
    type = vector_type(type, v->getType()->getVectorNumElements());
  }
  return _builder.CreateSIToFP(v, type, "wrld");
}

llvm::Value* IrGenerator::w2i(llvm::Value* v)
{
  auto& b = _builder;

  llvm::Type* back_type = world_type();
  llvm::Type* type = int_type();
  llvm::Constant* zero = constant_world(0);
  if (v->getType()->isVectorTy()) {
    y::size n = v->getType()->getVectorNumElements();
    back_type = vector_type(back_type, n);
    type = vector_type(type, n);
    zero = constant_vector(zero, n);
  }
  // Mathematical floor. Implements the algorithm:
  // return int(v) - (v < 0 && v != float(int(v)));
  auto cast = b.CreateFPToSI(v, type, "int");
  auto back = b.CreateSIToFP(cast, back_type, "int");

  auto a_check = b.CreateFCmpOLT(v, zero, "int");
  auto b_check = b.CreateFCmpONE(v, back, "int");
  return b.CreateSub(cast,
      b2i(b.CreateAnd(a_check, b_check, "int")), "int");
}

llvm::Value* IrGenerator::global_ptr(llvm::Value* ptr, y::size index)
{
  // The first index indexes the global data pointer itself, i.e. to obtain
  // the one and only global data structure at that memory location.
  y::vector<llvm::Value*> indexes{constant_int(0), constant_int(index)};
  llvm::Value* v = _builder.CreateGEP(ptr, indexes, "global");
  return v;
}

llvm::Value* IrGenerator::global_ptr(const y::string& name)
{
  return global_ptr(_metadata[GLOBAL_DATA_PTR], _global_numbering[name]);
}

llvm::Value* IrGenerator::pow(llvm::Value* v, llvm::Value* u)
{
  auto& b = _builder;
  llvm::Type* t = v->getType();

  y::vector<llvm::Type*> args{world_type(), world_type()};
  auto pow_ptr = get_native_function(
      "pow", (void_fp)&::pow,
      llvm::FunctionType::get(world_type(), args, false));

  if (t->isIntOrIntVectorTy()) {
    v = i2w(v);
    u = i2w(u);
  }

  if (!t->isVectorTy()) {
    y::vector<llvm::Value*> args{v, u};
    llvm::Value* r = b.CreateCall(pow_ptr, args, "pow");
    return t->isIntOrIntVectorTy() ? w2i(r) : r;
  }

  llvm::Value* result =
      constant_vector(constant_world(0), t->getVectorNumElements());
  for (y::size i = 0; i < t->getVectorNumElements(); ++i) {
    llvm::Value* x = b.CreateExtractElement(v, constant_int(i), "pow");
    llvm::Value* y = b.CreateExtractElement(u, constant_int(i), "pow");
    y::vector<llvm::Value*> args{x, y};

    llvm::Value* call = b.CreateCall(pow_ptr, args, "pow");
    result = b.CreateInsertElement(result, call, constant_int(i), "pow");
  }
  return t->isIntOrIntVectorTy() ? w2i(result) : result;
}

llvm::Value* IrGenerator::mod(llvm::Value* v, llvm::Value* u)
{
  auto& b = _builder;
  if (!v->getType()->isIntOrIntVectorTy()) {
    return b.CreateFRem(v, u, "fmod");
  }

  // Implements the following algorithm:
  // return (v >= 0 ? v : v + (bool(|v| % |u|) + |v| / |u|) * |u|) % |u|;
  // There are simpler ways, but they are vulnerable to overflow errors.
  // k = |v| % |u| + |v| / |u| is the smallest postive integer such that
  // k * |u| >= |v|.
  auto v_check = b.CreateICmpSGE(v, constant_int(0), "mod");
  auto u_check = b.CreateICmpSGE(u, constant_int(0), "mod");
  auto v_abs = b.CreateSelect(
      v_check, v, b.CreateSub(constant_int(0), v, "mod"), "mod");
  auto u_abs = b.CreateSelect(
      u_check, u, b.CreateSub(constant_int(0), u, "mod"), "mod");

  auto k = b.CreateAdd(b2i(i2b(b.CreateSRem(v_abs, u_abs, "mod"))),
                       b.CreateSDiv(v_abs, u_abs, "mod"), "mod");
  auto lhs = b.CreateSelect(
      v_check, v,
      b.CreateAdd(v, b.CreateMul(k, u_abs, "mod"), "mod"), "mod");
  return b.CreateSRem(lhs, u_abs, "mod");
}

llvm::Value* IrGenerator::div(llvm::Value* v, llvm::Value* u)
{
  auto& b = _builder;
  if (!v->getType()->isIntOrIntVectorTy()) {
    return b.CreateFDiv(v, u, "fdiv");
  }

  // Implements the following algorithm:
  // bool sign = (v < 0) == (u < 0);
  // int t = (v < 0 ? -(1 + v) : v) / |u|;
  // return (sign ? t : -(1 + t)) + (u < 0);
  auto v_check = b.CreateICmpSLT(v, constant_int(0), "div");
  auto u_check = b.CreateICmpSLT(u, constant_int(0), "div");
  auto sign = b.CreateICmpEQ(v_check, u_check, "div");
  auto u_abs = b.CreateSelect(
      u_check, b.CreateSub(constant_int(0), u, "div"), u, "div");

  auto t = b.CreateSelect(
      v_check,
      b.CreateSub(constant_int(-1), v, "div"), v, "div");
  t = b.CreateSDiv(t, u_abs, "div");
  return b.CreateAdd(
      b.CreateSelect(sign, t, b.CreateSub(constant_int(-1), t, "div"), "div"),
      b2i(u_check), "div");
}

llvm::Value* IrGenerator::binary(
    llvm::Value* left, llvm::Value* right,
    y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op)
{
  llvm::Type* l_type = left->getType();
  llvm::Type* r_type = right->getType();

  // If both scalar or vector, sizes must be equal, and we can directly operate
  // on the values.
  if (l_type->isVectorTy() == r_type->isVectorTy()) {
    return op(left, right);
  }

  // Otherwise one is a scalar and one a vector (but having the same base type),
  // and we need to extend the scalar to match the size of the vector.
  bool is_left = l_type->isVectorTy();
  y::size size = is_left ?
      l_type->getVectorNumElements() : r_type->getVectorNumElements();

  llvm::Value* v = y::null;
  if (l_type->isIntOrIntVectorTy()) {
    v = constant_vector(constant_int(0), size);
    // Can't insert booleans into a vector of int_type()!
    llvm::Type* i = is_left ? l_type->getVectorElementType() : l_type;
    if (i->getIntegerBitWidth() == 1) {
      v = i2b(v);
    }
  }
  else {
    v = constant_vector(constant_world(0), size);
  }

  for (y::size i = 0; i < size; ++i) {
    v = _builder.CreateInsertElement(
        v, is_left ? right : left, constant_int(i), "vec");
  }
  return is_left ? op(left, v) : op(v, right);
}

llvm::Value* IrGenerator::fold(
    llvm::Value* value,
    y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op,
    bool to_bool, bool with_ands, bool right_assoc)
{
  // Convert each argument to boolean, if necessary.
  y::vector<llvm::Value*> elements;
  for (y::size i = 0; i < value->getType()->getVectorNumElements(); ++i) {
    llvm::Value* v =
        _builder.CreateExtractElement(value, constant_int(i), "idx");
    elements.push_back(to_bool ? i2b(v) : v);
  }

  // Usually, we just form the chain (((e0 op e1) op e2) ...).
  if (!with_ands) {
    if (right_assoc) {
      auto it = elements.rbegin();
      llvm::Value* v = *it++;
      for (; it != elements.rend(); ++it) {
        v = op(*it, v);
      }
      return v;
    }
    auto it = elements.begin();
    llvm::Value* v = *it++;
    for (; it != elements.end(); ++it) {
      v = op(*it, v);
    }
    return v;
  }

  // For comparisons that isn't very useful, so instead form the chain
  // (e0 op e1) && (e1 op e2) && ...
  y::vector<llvm::Value*> comparisons;
  for (y::size i = 1; i < elements.size(); ++i) {
    comparisons.push_back(op(elements[i - 1], elements[i]));
  }

  // Ignore right_assoc, since logical AND is associative.
  llvm::Value* v = comparisons[0];
  for (y::size i = 1; i < comparisons.size(); ++i) {
    v = _builder.CreateAnd(v, comparisons[i], "fold");
  }
  return v;
}

llvm::Type* IrGenerator::get_llvm_type(const yang::Type& t) const
{
  if (t.is_function()) {
    y::vector<llvm::Type*> args;
    args.push_back(_global_data);
    for (y::size i = 0; i < t.get_function_num_args(); ++i) {
      args.push_back(get_llvm_type(t.get_function_arg_type(i)));
    }
    return llvm::PointerType::get(
        llvm::FunctionType::get(
            get_llvm_type(t.get_function_return_type()), args, false), 0);
  }
  if (t.is_int()) {
    return int_type();
  }
  if (t.is_world()) {
    return world_type();
  }
  if (t.is_int_vector()) {
    return vector_type(int_type(), t.get_vector_size());
  }
  if (t.is_world_vector()) {
    return vector_type(world_type(), t.get_vector_size());
  }
  return void_type();
}

llvm::BasicBlock* IrGenerator::create_block(
    metadata meta, const y::string& name)
{
  auto& b = _builder;
  auto parent = b.GetInsertBlock() ? b.GetInsertBlock()->getParent() : y::null;
  auto block = llvm::BasicBlock::Create(b.getContext(), name, parent);
  _metadata.add(meta, block);
  return block;
}

// End namespace yang::internal.
}
}
