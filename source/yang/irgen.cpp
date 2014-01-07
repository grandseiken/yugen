#include "irgen.h"
#include <llvm/IR/Module.h>
#include "../log.h"

namespace llvm {
  class LLVMContext;
}

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

IrGenerator::IrGenerator(llvm::Module& module,
                         const SymbolTable<Type>::scope& globals)
  : _module(module)
  , _builder(module.getContext())
  , _symbol_table(y::null)
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
}

void IrGenerator::emit_global_functions()
{
  auto& b = _builder;

  // Register malloc and free. Malloc takes a pointer argument, since
  // we computer sizeof with pointer arithmetic. (Conveniently, it's
  // also a type of the correct bit-width.)
  auto malloc_ptr = llvm::Function::Create(
      llvm::FunctionType::get(_global_data, _global_data, false),
      llvm::Function::ExternalLinkage, "malloc", &_module);
  auto free_ptr = llvm::Function::Create(
      llvm::FunctionType::get(void_type(), _global_data, false),
      llvm::Function::ExternalLinkage, "free", &_module);

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

    auto getter = llvm::Function::Create(
        llvm::FunctionType::get(t, _global_data, false),
        llvm::Function::ExternalLinkage, "!global_get_" + pair.first, &_module);
    auto getter_block = llvm::BasicBlock::Create(
        b.getContext(), "entry", getter);
    auto it = getter->arg_begin();
    it->setName("global");
    b.SetInsertPoint(getter_block);
    b.CreateRet(
        b.CreateLoad(global_ptr(it, pair.second), "load"));

    y::vector<llvm::Type*> setter_args{_global_data, t};
    auto setter = llvm::Function::Create(
        llvm::FunctionType::get(void_type(), setter_args, false),
        llvm::Function::ExternalLinkage, "!global_set_" + pair.first, &_module);
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

void IrGenerator::preorder(const Node& node)
{
  auto& b = _builder;
  auto parent = b.GetInsertBlock() ? b.GetInsertBlock()->getParent() : y::null;

  switch (node.type) {
    case Node::GLOBAL:
    {
      _symbol_table.push();
      _symbol_table.add("%GLOBAL%", y::null);

      // GLOBAL init functions don't need external linkage, since they are
      // called automatically by the externally-visible global structure
      // allocation function.
      auto function = llvm::Function::Create(
          llvm::FunctionType::get(void_type(), _global_data, false),
          llvm::Function::InternalLinkage, "global_init", &_module);
      _global_inits.push_back(function);
      auto block = llvm::BasicBlock::Create(b.getContext(), "entry", function);

      b.SetInsertPoint(block);
      _symbol_table.push();

      // Store a special entry in the symbol table for the implicit global
      // structure pointer.
      auto it = function->arg_begin();
      it->setName("global");
      _symbol_table.add("%GLOBAL_DATA%", it);
      _symbol_table.add("%CURRENT_FUNCTION%", function);
      break;
    }

    case Node::BLOCK:
      _symbol_table.push();
      break;

    case Node::IF_STMT:
    {
      _symbol_table.push();
      auto then_block = llvm::BasicBlock::Create(b.getContext(), "then");
      auto else_block = llvm::BasicBlock::Create(b.getContext(), "else");
      auto merge_block = llvm::BasicBlock::Create(b.getContext(), "merge");
      _symbol_table.add("%IF_STMT_THEN_BLOCK%", then_block);
      _symbol_table.add("%IF_STMT_ELSE_BLOCK%", else_block);
      _symbol_table.add("%IF_STMT_MERGE_BLOCK%", merge_block);
      break;
    }

    case Node::FOR_STMT:
    {
      _symbol_table.push();
      auto cond_block = llvm::BasicBlock::Create(b.getContext(), "cond");
      auto loop_block = llvm::BasicBlock::Create(b.getContext(), "loop");
      auto after_block = llvm::BasicBlock::Create(b.getContext(), "after");
      auto merge_block = llvm::BasicBlock::Create(b.getContext(), "merge");
      _symbol_table.add("%FOR_STMT_COND_BLOCK%", cond_block);
      _symbol_table.add("%FOR_STMT_LOOP_BLOCK%", loop_block);
      _symbol_table.add("%FOR_STMT_MERGE_BLOCK%", merge_block);
      _symbol_table.add("%FOR_STMT_AFTER_BLOCK%", after_block);
      _symbol_table.add("%LOOP_BREAK_BLOCK%", merge_block);
      _symbol_table.add("%LOOP_CONTINUE_BLOCK%", after_block);
      break;
    }

    case Node::DO_WHILE_STMT:
    {
      _symbol_table.push();
      auto loop_block = llvm::BasicBlock::Create(b.getContext(), "loop");
      auto cond_block = llvm::BasicBlock::Create(b.getContext(), "cond");
      auto merge_block = llvm::BasicBlock::Create(b.getContext(), "merge");
      _symbol_table.add("%DO_WHILE_STMT_LOOP_BLOCK%", loop_block);
      _symbol_table.add("%DO_WHILE_STMT_COND_BLOCK%", cond_block);
      _symbol_table.add("%DO_WHILE_STMT_MERGE_BLOCK%", merge_block);
      _symbol_table.add("%LOOP_BREAK_BLOCK%", merge_block);
      _symbol_table.add("%LOOP_CONTINUE_BLOCK%", cond_block);

      b.CreateBr(loop_block);
      parent->getBasicBlockList().push_back(loop_block);
      b.SetInsertPoint(loop_block);
      break;
    }

    default: {}
  }
}

void IrGenerator::infix(const Node& node, const result_list& results)
{
  auto& b = _builder;
  auto parent = b.GetInsertBlock() ? b.GetInsertBlock()->getParent() : y::null;

  switch (node.type) {
    case Node::FUNCTION:
    {
      auto function_type = (llvm::FunctionType*)results[0].type;
      // Linkage will be set to external later for exported top-level functions.
      auto function = llvm::Function::Create(
          function_type, llvm::Function::InternalLinkage,
          "anonymous", &_module);
      auto block = llvm::BasicBlock::Create(b.getContext(), "entry", function);

      b.SetInsertPoint(block);
      _symbol_table.push();

      // The code for Node::TYPE_FUNCTION in visit() ensures it takes a global
      // data structure pointer.
      auto it = function->arg_begin();
      it->setName("global");
      _symbol_table.add("%GLOBAL_DATA%", it);
      // Set up the arguments.
      y::size arg_num = 0;
      for (++it; it != function->arg_end(); ++it) {
        const y::string& name =
            node.children[0]->children[1 + arg_num]->string_value;
        it->setName(name);

        // Rather than reference argument values directly, we create an alloca
        // and store the argument in there. This simplifies things, since we
        // can emit the same IR code when referencing local variables or
        // function arguments. 
        llvm::Value* v = b.CreateAlloca(
            function_type->getParamType(1 + arg_num), y::null, name);
        b.CreateStore(it, v);
        _symbol_table.add(name, v);
        ++arg_num;
      }
      _symbol_table.add("%CURRENT_FUNCTION%", function);
      break;
    }

    case Node::IF_STMT:
    {
      auto then_block =
          (llvm::BasicBlock*)_symbol_table["%IF_STMT_THEN_BLOCK%"];
      auto else_block =
          (llvm::BasicBlock*)_symbol_table["%IF_STMT_ELSE_BLOCK%"];
      auto merge_block =
          (llvm::BasicBlock*)_symbol_table["%IF_STMT_MERGE_BLOCK%"];

      if (results.size() == 1) {
        bool has_else = node.children.size() > 2;
        b.CreateCondBr(i2b(results[0]), then_block,
                       has_else ? else_block : merge_block);
        parent->getBasicBlockList().push_back(then_block);
        b.SetInsertPoint(then_block);
      }
      if (results.size() == 2) {
        b.CreateBr(merge_block);
        parent->getBasicBlockList().push_back(else_block);
        b.SetInsertPoint(else_block);
      }
      break;
    }

    case Node::FOR_STMT:
    {
      auto cond_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_COND_BLOCK%"];
      auto loop_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_LOOP_BLOCK%"];
      auto merge_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_MERGE_BLOCK%"];
      auto after_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_AFTER_BLOCK%"];

      if (results.size() == 1) {
        b.CreateBr(cond_block);
        parent->getBasicBlockList().push_back(cond_block);
        b.SetInsertPoint(cond_block);
      }
      if (results.size() == 2) {
        b.CreateCondBr(i2b(results[1]), loop_block, merge_block);
        parent->getBasicBlockList().push_back(after_block);
        b.SetInsertPoint(after_block);
      }
      if (results.size() == 3) {
        b.CreateBr(cond_block);
        parent->getBasicBlockList().push_back(loop_block);
        b.SetInsertPoint(loop_block);
      }
      break;
    }

    case Node::DO_WHILE_STMT:
    {
      auto cond_block =
          (llvm::BasicBlock*)_symbol_table["%DO_WHILE_STMT_COND_BLOCK%"];

      b.CreateBr(cond_block);
      parent->getBasicBlockList().push_back(cond_block);
      b.SetInsertPoint(cond_block);
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
             type == Node::MOD ? b.CreateSRem(v, u, "mod") :
             type == Node::ADD ? b.CreateAdd(v, u, "add") :
             type == Node::SUB ? b.CreateSub(v, u, "sub") :
             type == Node::MUL ? b.CreateMul(v, u, "mul") :
             type == Node::DIV ? b.CreateSDiv(v, u, "div") :
             type == Node::EQ ? b.CreateICmpEQ(v, u, "eq") :
             type == Node::NE ? b.CreateICmpNE(v, u, "ne") :
             type == Node::GE ? b.CreateICmpSGE(v, u, "ge") :
             type == Node::LE ? b.CreateICmpSLE(v, u, "le") :
             type == Node::GT ? b.CreateICmpSGT(v, u, "gt") :
             type == Node::LT ? b.CreateICmpSLT(v, u, "lt") :
             y::null;
    }
    return type == Node::MOD ? b.CreateFRem(v, u, "fmod") :
           type == Node::ADD ? b.CreateFAdd(v, u, "fadd") :
           type == Node::SUB ? b.CreateFSub(v, u, "fsub") :
           type == Node::MUL ? b.CreateFMul(v, u, "fmul") :
           type == Node::DIV ? b.CreateFDiv(v, u, "fdiv") :
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
      _symbol_table.pop();
      return results[0];
    case Node::GLOBAL_ASSIGN:
    {
      // Top-level functions Nodes have their int_value set to 1 when defined
      // using the `export` keyword.
      auto function = (llvm::Function*)parent;
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
      // If this was a nested function, set the insert point back to the last
      // block in the enclosing function.
      if (_symbol_table.has("%CURRENT_FUNCTION%")) {
        auto function = (llvm::Function*)_symbol_table["%CURRENT_FUNCTION%"];
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
      auto merge_block =
          (llvm::BasicBlock*)_symbol_table["%IF_STMT_MERGE_BLOCK%"];
      _symbol_table.pop();
      b.CreateBr(merge_block);
      parent->getBasicBlockList().push_back(merge_block);
      b.SetInsertPoint(merge_block);
      return results[0];
    }
    case Node::FOR_STMT:
    {
      auto after_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_AFTER_BLOCK%"];
      auto merge_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_MERGE_BLOCK%"];
      _symbol_table.pop();

      b.CreateBr(after_block);
      parent->getBasicBlockList().push_back(merge_block);
      b.SetInsertPoint(merge_block);
      return results[0];
    }
    case Node::DO_WHILE_STMT:
    {
      auto loop_block =
          (llvm::BasicBlock*)_symbol_table["%DO_WHILE_STMT_LOOP_BLOCK%"];
      auto merge_block =
          (llvm::BasicBlock*)_symbol_table["%DO_WHILE_STMT_MERGE_BLOCK%"];
      _symbol_table.pop();

      b.CreateCondBr(i2b(results[1]), loop_block, merge_block);
      parent->getBasicBlockList().push_back(merge_block);
      b.SetInsertPoint(merge_block);
      return results[0];
    }
    case Node::BREAK_STMT:
    case Node::CONTINUE_STMT:
    {
      auto dead_block =
          llvm::BasicBlock::Create(b.getContext(), "dead", parent);
      llvm::Value* v = b.CreateBr((llvm::BasicBlock*)_symbol_table[
          node.type == Node::BREAK_STMT ? "%LOOP_BREAK_BLOCK%" :
                                          "%LOOP_CONTINUE_BLOCK%"]);
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
      // TODO: this evaluates both arguments. Should definitely branch.
      // Furthermore, logical AND and logical OR are currently using
      // select instead of branching. This is bad.
      return b.CreateSelect(i2b(results[0]), results[1], results[2]);
    case Node::CALL:
    {
      y::vector<llvm::Value*> args;
      args.push_back(_symbol_table["%GLOBAL_DATA%"]);
      for (y::size i = 1; i < results.size(); ++i) {
        args.push_back(results[i]);
      }
      // No name, since it may have type VOID.
      return b.CreateCall(results[0], args);
    }


    // Logical OR and AND short-circuit. This interacts in a subtle way with
    // vectorisation: we can short-circuit only when the left-hand operand is a
    // primitive. (We could also check and short-circuit if the left-hand
    // operand is an entirely zero or entirely non-zero vector. We currently
    // don't.)
    case Node::LOGICAL_OR:
    case Node::LOGICAL_AND:
    {
      if (types[0]->isVectorTy()) {
        return b2i(binary(i2b(results[0]), i2b(results[1]), binary_lambda));
      }
      llvm::Constant* constant =
          constant_int(node.type == Node::LOGICAL_OR ? 1 : 0);
      if (types[1]->isVectorTy()) {
        constant = constant_vector(constant, types[1]->getVectorNumElements());
      }
      return node.type == Node::LOGICAL_OR ?
          b.CreateSelect(i2b(results[0]), constant, b2i(i2b(results[1]))) :
          b.CreateSelect(i2b(results[0]), b2i(i2b(results[1])), constant);
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
      // TODO: implement POW.
      return results[0];
    case Node::MOD:
    case Node::ADD:
    case Node::SUB:
    case Node::MUL:
    case Node::DIV:
      // TODO: euclidean mod and euclidean division.
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
      // TODO: implement FOLD_POW.
    case Node::FOLD_MOD:
    case Node::FOLD_ADD:
    case Node::FOLD_SUB:
    case Node::FOLD_MUL:
    case Node::FOLD_DIV:
      // TODO: euclidean mod and division.
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
      if (_symbol_table.has("%GLOBAL%")) {
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
    {
      llvm::Type* type = int_type();
      if (types[0]->isVectorTy()) {
        type = vector_type(type, types[0]->getVectorNumElements());
      }
      // TODO: mathematical floor.
      return b.CreateFPToSI(results[0], type, "int");
    }
    case Node::WORLD_CAST:
    {
      llvm::Type* type = world_type();
      if (types[0]->isVectorTy()) {
        type = vector_type(type, types[0]->getVectorNumElements());
      }
      return b.CreateSIToFP(results[0], type, "wrld");
    }

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
      // TODO: vector index range-checking.
      return b.CreateExtractElement(results[0], results[1], "idx");

    default:
      return (llvm::Value*)y::null;
  }
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
  return global_ptr(_symbol_table["%GLOBAL_DATA%"], _global_numbering[name]);
}

llvm::Value* IrGenerator::binary(
    llvm::Value* left, llvm::Value* right,
    y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op)
{
  // If both scalar or vector, sizes must be equal, and we can directly operate
  // on the values.
  if (left->getType()->isVectorTy() == right->getType()->isVectorTy()) {
    return op(left, right);
  }

  // Otherwise one is a scalar and one a vector (but having the same base type),
  // and we need to extend the scalar to match the size of the vector.
  bool is_left = left->getType()->isVectorTy();
  y::size size = is_left ?
      left->getType()->getVectorNumElements() :
      right->getType()->getVectorNumElements();

  llvm::Value* v = constant_vector(
      left->getType()->isIntOrIntVectorTy() ?
      constant_int(0) : constant_world(0), size);

  for (y::size i = 0; i < size; ++i) {
    v = _builder.CreateInsertElement(
        v, is_left ? right : left, constant_int(i), "vec");
  }
  return is_left ? op(left, v) : op(v, right);
}

llvm::Value* IrGenerator::fold(
    llvm::Value* value,
    y::function<llvm::Value*(llvm::Value*, llvm::Value*)> op,
    bool to_bool, bool with_ands)
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
    llvm::Value* v = elements[0];
    for (y::size i = 1; i < elements.size(); ++i) {
      v = op(v, elements[i]);
    }
    return v;
  }

  // For comparisons that isn't very useful, so instead form the chain
  // (e0 op e1) && (e1 op e2) && ...
  y::vector<llvm::Value*> comparisons;
  for (y::size i = 1; i < elements.size(); ++i) {
    comparisons.push_back(op(elements[i - 1], elements[i]));
  }

  llvm::Value* v = comparisons[0];
  for (y::size i = 1; i < comparisons.size(); ++i) {
    v = _builder.CreateAnd(v, comparisons[i], "fold");
  }
  return v;
}

llvm::Type* IrGenerator::get_llvm_type(const Type& t) const
{
  if (t.function()) {
    y::vector<llvm::Type*> args;
    args.push_back(_global_data);
    for (y::size i = 1; i < t.element_size(); ++i) {
      args.push_back(get_llvm_type(t.elements(i)));
    }
    return llvm::PointerType::get(
        llvm::FunctionType::get(get_llvm_type(t.elements(0)), args, false), 0);
  }
  if (t.is_int()) {
    return t.is_vector() ? vector_type(int_type(), t.count()) : int_type();
  }
  if (t.is_world()) {
    return t.is_vector() ?
        vector_type(world_type(), t.count()) : world_type();
  }
  return void_type();
}
