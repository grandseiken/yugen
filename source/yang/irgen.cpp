#include "irgen.h"

#include <llvm/IR/Module.h>

namespace llvm {
  class LLVMContext;
}

IrGenerator::IrGenerator(llvm::Module& module)
  : _module(module)
  , _builder(module.getContext())
{
}

IrGenerator::~IrGenerator()
{
}

void IrGenerator::generate(const Node& node)
{
  auto function = llvm::Function::Create(
      llvm::FunctionType::get(int_type(), false),
      llvm::Function::ExternalLinkage, "main", &_module);
  auto block = llvm::BasicBlock::Create(
      _builder.getContext(), "entry", function);

  _builder.SetInsertPoint(block);
  _builder.CreateRet(walk(node));
}

llvm::Value* IrGenerator::visit(const Node& node, const result_list& results)
{
  auto& b = _builder;

  switch (node.type) {
    case Node::IDENTIFIER:
      // TODO.
      return constant_int(0);

    case Node::INT_LITERAL:
      return constant_int(node.int_value);
    case Node::WORLD_LITERAL:
      return constant_world(node.world_value);

    case Node::TERNARY:
      return branch(results[0], results[1], results[2]);

    case Node::LOGICAL_OR:
      return b2i(b.CreateOr(i2b(results[0]), i2b(results[1]), "lor"));
    case Node::LOGICAL_AND:
      return b2i(b.CreateAnd(i2b(results[0]), i2b(results[1]), "land"));
    case Node::BITWISE_OR:
      return b.CreateOr(results[0], results[1], "or");
    case Node::BITWISE_AND:
      return b.CreateAnd(results[0], results[1], "and");
    case Node::BITWISE_XOR:
      return b.CreateXor(results[0], results[1], "xor");
    case Node::BITWISE_LSHIFT:
      return b.CreateShl(results[0], results[1], "lsh");
    case Node::BITWISE_RSHIFT:
      return b.CreateAShr(results[0], results[1], "rsh");

    case Node::POW:
      // TODO.
      return results[0];
    case Node::MOD:
      // TODO: euclidean mod.
      return results[0]->getType()->isIntegerTy() ?
          b.CreateSRem(results[0], results[1], "mod") :
          b.CreateFRem(results[0], results[1], "fmod");
    case Node::ADD:
      return results[0]->getType()->isIntegerTy() ?
          b.CreateAdd(results[0], results[1], "add") :
          b.CreateFAdd(results[0], results[1], "fadd");
    case Node::SUB:
      return results[0]->getType()->isIntegerTy() ?
          b.CreateSub(results[0], results[1], "sub") :
          b.CreateFSub(results[0], results[1], "fsub");
    case Node::MUL:
      return results[0]->getType()->isIntegerTy() ?
          b.CreateMul(results[0], results[1], "mul") :
          b.CreateFMul(results[0], results[1], "fmul");
    case Node::DIV:
      // TODO: euclidean division.
      return results[0]->getType()->isIntegerTy() ?
          b.CreateSDiv(results[0], results[1], "div") :
          b.CreateFDiv(results[0], results[1], "fdiv");

    case Node::EQ:
      return b2i(results[0]->getType()->isIntegerTy() ?
          b.CreateICmpEQ(results[0], results[1], "eq") :
          b.CreateFCmpOEQ(results[0], results[1], "feq"));
    case Node::NE:
      return b2i(results[0]->getType()->isIntegerTy() ?
          b.CreateICmpNE(results[0], results[1], "ne") :
          b.CreateFCmpONE(results[0], results[1], "fne"));
    case Node::GE:
      return b2i(results[0]->getType()->isIntegerTy() ?
          b.CreateICmpSGE(results[0], results[1], "ge") :
          b.CreateFCmpOGE(results[0], results[1], "fge"));
    case Node::LE:
      return b2i(results[0]->getType()->isIntegerTy() ?
          b.CreateICmpSLE(results[0], results[1], "le") :
          b.CreateFCmpOLE(results[0], results[1], "fle"));
    case Node::GT:
      return b2i(results[0]->getType()->isIntegerTy() ?
          b.CreateICmpSGT(results[0], results[1], "gt") :
          b.CreateFCmpOGT(results[0], results[1], "fgt"));
    case Node::LT:
      return b2i(results[0]->getType()->isIntegerTy() ?
          b.CreateICmpSLT(results[0], results[1], "lt") :
          b.CreateFCmpOLT(results[0], results[1], "flt"));

    case Node::LOGICAL_NEGATION:
      return b2i(b.CreateICmpEQ(results[0], constant_int(0), "lneg"));
    case Node::BITWISE_NEGATION:
      return b.CreateXor(
          results[0], constant_int((unsigned y::int32)(0) - 1), "neg");
    case Node::ARITHMETIC_NEGATION:
      return results[0]->getType()->isIntegerTy() ?
          b.CreateSub(constant_int(0), results[0], "sub") :
          b.CreateFSub(constant_world(0), results[0], "fsub");
    case Node::INT_CAST:
      // TODO: mathematical floor.
      return b.CreateFPToSI(results[0], int_type(), "int");
    case Node::WORLD_CAST:
      return b.CreateSIToFP(results[0], world_type(), "wrld");
  }

  return y::null;
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

llvm::Value* IrGenerator::constant_int(y::int32 value) const
{
  return llvm::ConstantInt::getSigned(int_type(), value);
}

llvm::Value* IrGenerator::constant_world(y::world value) const
{
  return llvm::ConstantFP::get(_builder.getContext(), llvm::APFloat(value));
}

llvm::Value* IrGenerator::i2b(llvm::Value* v)
{
  return _builder.CreateICmpNE(v, constant_int(0), "bool");
}

llvm::Value* IrGenerator::b2i(llvm::Value* v)
{
  return _builder.CreateZExt(v, int_type(), "int");
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
