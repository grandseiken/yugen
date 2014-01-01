#include "irgen.h"
#include <llvm/IR/Module.h>

namespace llvm {
  class LLVMContext;
}

IrGenerator::IrGenerator(llvm::Module& module)
  : _module(module)
  , _builder(module.getContext())
  , _symbol_table(y::null)
{
}

IrGenerator::~IrGenerator()
{
}

void IrGenerator::preorder(const Node& node)
{
  auto& b = _builder;

  switch (node.type) {
    case Node::FUNCTION:
    {
      auto function = llvm::Function::Create(
          llvm::FunctionType::get(int_type(), false),
          llvm::Function::ExternalLinkage, node.string_value, &_module);
      auto block = llvm::BasicBlock::Create(b.getContext(), "entry", function);

      b.SetInsertPoint(block);
      _symbol_table.push();
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
      break;
    }

    default: {}
  }
}

void IrGenerator::infix(const Node& node, const result_list& results)
{
  auto& b = _builder;
  switch (node.type) {
    case Node::IF_STMT:
    {
      auto parent = b.GetInsertBlock()->getParent();

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
      auto parent = b.GetInsertBlock()->getParent();

      auto cond_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_COND_BLOCK%"];
      auto loop_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_LOOP_BLOCK%"];
      auto merge_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_MERGE_BLOCK%"];
      auto after_block =
          (llvm::BasicBlock*)_symbol_table["%FOR_STMT_AFTER_BLOCK%"];

      if (results.size() == 1) {
        auto parent = b.GetInsertBlock()->getParent();
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

    default: {}
  }
}

llvm::Value* IrGenerator::visit(const Node& node, const result_list& results)
{
  auto& b = _builder;
  y::vector<llvm::Type*> types;
  for (llvm::Value* v : results) {
    types.push_back(v->getType());
  }

  switch (node.type) {
    case Node::FUNCTION:
      _builder.CreateBr(_builder.GetInsertBlock());
      _symbol_table.pop();
      return results[0];

    case Node::BLOCK:
    {
      auto parent = b.GetInsertBlock()->getParent();
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
      auto parent = b.GetInsertBlock()->getParent();
      auto dead_block =
          llvm::BasicBlock::Create(b.getContext(), "dead", parent);
      llvm::Value* v = b.CreateRet(results[0]);
      b.SetInsertPoint(dead_block);
      return v;
    }
    case Node::IF_STMT:
    {
      auto parent = b.GetInsertBlock()->getParent();
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
      auto parent = b.GetInsertBlock()->getParent();
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
    case Node::BREAK_STMT:
    case Node::CONTINUE_STMT:
    {
      auto parent = b.GetInsertBlock()->getParent();
      auto dead_block =
          llvm::BasicBlock::Create(b.getContext(), "dead", parent);
      llvm::Value* v = b.CreateBr((llvm::BasicBlock*)_symbol_table[
          node.type == Node::BREAK_STMT ? "%FOR_STMT_MERGE_BLOCK%" :
                                          "%FOR_STMT_AFTER_BLOCK%"]);
      b.SetInsertPoint(dead_block);
      return v;
    }

    case Node::IDENTIFIER:
      return b.CreateLoad(_symbol_table[node.string_value], "load");

    case Node::INT_LITERAL:
      return constant_int(node.int_value);
    case Node::WORLD_LITERAL:
      return constant_world(node.world_value);

    case Node::TERNARY:
      return branch(results[0], results[1], results[2]);

    // Logical OR and AND short-circuit. This interacts in a subtle way with
    // vectorisation: we can short-circuit only when the left-hand operand is a
    // primitive. (We could also check and short-circuit if the left-hand
    // operand is an entirely zero or entirely non-zero vector. We currently
    // don't.)
    case Node::LOGICAL_OR:
    {
      if (types[0]->isVectorTy()) {
        return b2i(binary(
            results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
        {
          return b.CreateOr(i2b(v), i2b(u), "lor");
        }));
      }
      llvm::Constant* constant = constant_int(1);
      if (types[1]->isVectorTy()) {
        constant = constant_vector(constant, types[1]->getVectorNumElements());
      }
      return branch(results[0], constant, b2i(i2b(results[1])));
    }
    case Node::LOGICAL_AND:
    {
      if (types[0]->isVectorTy()) {
        return b2i(binary(
            results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
        {
          return b.CreateAnd(i2b(v), i2b(u), "land");
        }));
      }
      llvm::Constant* constant = constant_int(0);
      if (types[1]->isVectorTy()) {
        constant = constant_vector(constant, types[1]->getVectorNumElements());
      }
      return branch(results[0], b2i(i2b(results[1])), constant);
    }

    // Bitwise functions map directly to (vectorisations of) LLVM IR
    // instructions.
    case Node::BITWISE_OR:
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateOr(v, u, "or");
      });
    case Node::BITWISE_AND:
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateAnd(v, u, "and");
      });
    case Node::BITWISE_XOR:
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateXor(v, u, "xor");
      });
    case Node::BITWISE_LSHIFT:
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateShl(v, u, "lsh");
      });
    case Node::BITWISE_RSHIFT:
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateAShr(v, u, "rsh");
      });

    case Node::POW:
      // TODO.
      return results[0];
    case Node::MOD:
      // TODO: euclidean mod.
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateSRem(v, u, "mod") : b.CreateFRem(v, u, "fmod");
      });
    case Node::ADD:
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateAdd(v, u, "add") : b.CreateFAdd(v, u, "fadd");
      });
    case Node::SUB:
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateSub(v, u, "sub") : b.CreateFSub(v, u, "fsub");
      });
    case Node::MUL:
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateMul(v, u, "mul") : b.CreateFMul(v, u, "fmul");
      });
    case Node::DIV:
      // TODO: euclidean division.
      return binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateSDiv(v, u, "div") : b.CreateFDiv(v, u, "fdib");
      });

    case Node::EQ:
      return b2i(
          binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateICmpEQ(v, u, "eq") : b.CreateFCmpOEQ(v, u, "feq");
      }));
    case Node::NE:
      return b2i(
          binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateICmpNE(v, u, "ne") : b.CreateFCmpONE(v, u, "fne");
      }));
    case Node::GE:
      return b2i(
          binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateICmpSGE(v, u, "ge") : b.CreateFCmpOGE(v, u, "fge");
      }));
    case Node::LE:
      return b2i(
          binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateICmpSLE(v, u, "le") : b.CreateFCmpOLE(v, u, "fle");
      }));
    case Node::GT:
      return b2i(
          binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateICmpSGT(v, u, "gt") : b.CreateFCmpOGT(v, u, "fgt");
      }));
    case Node::LT:
      return b2i(
          binary(results[0], results[1], [&](llvm::Value* v, llvm::Value* u)
      {
        return v->getType()->isIntOrIntVectorTy() ?
           b.CreateICmpSLT(v, u, "lt") : b.CreateFCmpOLT(v, u, "flt");
      }));

    case Node::FOLD_LOGICAL_OR:
      return b2i(fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateOr(v, u, "lor");
      }, true));
    case Node::FOLD_LOGICAL_AND:
      return b2i(fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateAnd(v, u, "land");
      }, true));
    case Node::FOLD_BITWISE_OR:
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateOr(v, u, "or");
      });
    case Node::FOLD_BITWISE_AND:
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateAnd(v, u, "and");
      });
    case Node::FOLD_BITWISE_XOR:
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateXor(v, u, "xor");
      });
    case Node::FOLD_BITWISE_LSHIFT:
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateShl(v, u, "lsh");
      });
    case Node::FOLD_BITWISE_RSHIFT:
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return b.CreateAShr(v, u, "rsh");
      });

    case Node::FOLD_POW:
      // TODO.
    case Node::FOLD_MOD:
      // TODO: euclidean mod.
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateSRem(v, u, "mod") : b.CreateFRem(v, u, "fmod");
      });
    case Node::FOLD_ADD:
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateAdd(v, u, "add") : b.CreateFAdd(v, u, "fadd");
      });
    case Node::FOLD_SUB:
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateSub(v, u, "sub") : b.CreateFSub(v, u, "fsub");
      });
    case Node::FOLD_MUL:
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateMul(v, u, "mul") : b.CreateFMul(v, u, "fmul");
      });
    case Node::FOLD_DIV:
      // TODO: euclidean division.
      return fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateSDiv(v, u, "div") : b.CreateFDiv(v, u, "fdiv");
      });

    case Node::FOLD_EQ:
      return b2i(fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateICmpEQ(v, u, "eq") : b.CreateFCmpOEQ(v, u, "feq");
      }, false, true));
    case Node::FOLD_NE:
      return b2i(fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateICmpNE(v, u, "ne") : b.CreateFCmpONE(v, u, "fne");
      }, false, true));
    case Node::FOLD_GE:
      return b2i(fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateICmpSGE(v, u, "ge") : b.CreateFCmpOGE(v, u, "fge");
      }, false, true));
    case Node::FOLD_LE:
      return b2i(fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateICmpSLE(v, u, "le") : b.CreateFCmpOLE(v, u, "fle");
      }, false, true));
    case Node::FOLD_GT:
      return b2i(fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateICmpSGT(v, u, "gt") : b.CreateFCmpOGT(v, u, "fgt");
      }, false, true));
    case Node::FOLD_LT:
      return b2i(fold(results[0], [&](llvm::Value* v, llvm::Value* u)
      {
        return types[0]->isIntOrIntVectorTy() ?
            b.CreateICmpSLT(v, u, "lt") : b.CreateFCmpOLT(v, u, "flt");
      }, false, true));

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
      llvm::Constant* cmp = results[0]->getType()->isIntOrIntVectorTy() ?
          constant_int(0) : constant_world(0);
      if (types[0]->isVectorTy()) {
        cmp = constant_vector(cmp, types[0]->getVectorNumElements());
      }
      return types[0]->isIntOrIntVectorTy() ?
          b.CreateSub(cmp, results[0], "sub") :
          b.CreateFSub(cmp, results[0], "fsub");
    }

    case Node::ASSIGN:
      b.CreateStore(results[0], _symbol_table[node.string_value]);
      return results[0];

    case Node::ASSIGN_VAR:
    {
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
      return b.CreateExtractElement(results[0], results[1], "idx");

    default:
      return y::null;
  }
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

llvm::Value* IrGenerator::branch(
    llvm::Value* cond, llvm::Value* left, llvm::Value* right)
{
  auto& b = _builder;
  auto parent = b.GetInsertBlock()->getParent();

  auto left_block = llvm::BasicBlock::Create(b.getContext(), "left", parent);
  auto right_block = llvm::BasicBlock::Create(b.getContext(), "right");
  auto merge_block = llvm::BasicBlock::Create(b.getContext(), "merge");

  b.CreateCondBr(i2b(cond), left_block, right_block);
  b.SetInsertPoint(left_block);
  b.CreateBr(merge_block);
  left_block = b.GetInsertBlock();

  parent->getBasicBlockList().push_back(right_block);
  b.SetInsertPoint(right_block);
  b.CreateBr(merge_block);
  right_block = b.GetInsertBlock();

  parent->getBasicBlockList().push_back(merge_block);
  b.SetInsertPoint(merge_block);
  auto phi = b.CreatePHI(left->getType(), 2, "branch");

  phi->addIncoming(left, left_block);
  phi->addIncoming(right, right_block);
  return phi;
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
