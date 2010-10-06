#include <tr1/unordered_map>
#include <vector>
#include <iostream>  // NOLINT
#include <functional>
#include <tr1/functional>
#include <algorithm>
#include "codegen.h"

namespace iv {
namespace core {

std::ostream& operator<<(std::ostream& os, const CodeGenerator& out) {
  return os << *(out.module());
}

CodeGenerator::CodeGenerator()
      : context_(llvm::getGlobalContext()),
        builder_(NULL),
        error_(),
        module_(NULL),
        provider_(NULL),
        engine_(NULL),
        int1ty_(llvm::Type::getInt1Ty(context_)),
        int8ty_(llvm::Type::getInt8Ty(context_)),
        int16ty_(llvm::Type::getInt16Ty(context_)),
        int32ty_(llvm::Type::getInt32Ty(context_)),
        int64ty_(llvm::Type::getInt64Ty(context_)),
        doublety_(llvm::Type::getDoubleTy(context_)),
        ptr8ty_(llvm::PointerType::getUnqual(int8ty_)),
        ptr32ty_(llvm::PointerType::getUnqual(int32ty_)),
        ptr64ty_(llvm::PointerType::getUnqual(int64ty_)),
        ptrptrty_(llvm::PointerType::getUnqual(ptr8ty_)),
        pointersizety_(NULL),
        ret_(),
        constant_strings_() {
    llvm::llvm_start_multithreaded();
    llvm::InitializeNativeTarget();
    module_ = new llvm::Module("iv.prelude", context_);
    provider_ = new llvm::ExistingModuleProvider(module_);
    engine_ = llvm::ExecutionEngine::create(
        provider_,
        false,
        &error_,
        llvm::CodeGenOpt::Aggressive,
        true);
    builder_ = new Builder(context_,
                           llvm::TargetFolder(engine_->getTargetData(),
                                              context_));
    Initialize();
}

void CodeGenerator::Initialize() {
  pointersizety_ = module_->getPointerSize() == llvm::Module::Pointer32 ?
      int32ty_ : int64ty_;
  std::vector<const llvm::Type*> StructTy_struct_iv_header;
  StructTy_struct_iv_header.push_back(ptr8ty_);
  module_->addTypeName("struct.iv::Header",
                       llvm::StructType::get(context_,
                                            StructTy_struct_iv_header,
                                            false));
  module_->addTypeName("struct.iv::Number", llvm::OpaqueType::get(context_));
  module_->addTypeName("struct.iv::Array", llvm::OpaqueType::get(context_));
}

CodeGenerator::~CodeGenerator() {
  delete builder_;
  delete engine_;
  llvm::llvm_shutdown();
}

void CodeGenerator::Visit(Block* block) {
}

void CodeGenerator::Visit(FunctionStatement* func) {
}

void CodeGenerator::Visit(VariableStatement* var) {
  std::for_each(var->decls().begin(),
                var->decls().end(),
                Visitor<CodeGenerator>(this));
}

void CodeGenerator::Visit(Declaration* decl) {
}

void CodeGenerator::Visit(EmptyStatement* empty) {
  // empty statement
  // do nothing
  ret_ = NULL;
}

void CodeGenerator::Visit(IfStatement* ifstmt) {
//  ifstmt->cond()->Accept(this);
//  llvm::Value* condv = IsJSTrue(ret_);
//  llvm::Function* now = builder_->GetInsertBlock()->getParent();
//  llvm::BasicBlock* thenblock = llvm::BasicBlock::Create(context_, "then", now);
//  llvm::BasicBlock* elseblock = llvm::BasicBlock::Create(context_, "else");
//  llvm::BasicBlock* mergeblock = llvm::BasicBlock::Create(context_, "ifcont");
//
//  builder_->CreateCondBr(condv, thenblock, elseblock);
//
//  // then
//  builder_->SetInsertPoint(thenblock);
//  // then block
//  ifstmt->then_statement()->Accept(this);
//  builder_->CreateBr(mergeblock);
//  thenblock = builder_->GetInsertBlock();
//
//  if (ifstmt->else_statement()) {
//    // else
//    now->getBasicBlockList().push_back(elseblock);
//    builder_->SetInsertPoint(elseblock);
//    // else block
//    ifstmt->else_statement()->Accept(this);
//    builder_->CreateBr(mergeblock);
//    elseblock = builder_->GetInsertBlock();
//  }
//
//  // merge
//  now->getBasicBlockList().push_back(mergeblock);
//  builder_->SetInsertPoint(mergeblock);
//  ret_ = NULL;
}

void CodeGenerator::Visit(DoWhileStatement* dowhile) {
}

void CodeGenerator::Visit(WhileStatement* whilestmt) {
//  llvm::Function* now = builder_->GetInsertBlock()->getParent();
//  llvm::BasicBlock* whileblock = llvm::BasicBlock::Create(context_, "block", now);
//
//  builder_->SetInsertPoint(whileblock);
//  whilestmt->cond()->Accept(this);
//
//  llvm::Value* condv = IsJSTrue(ret_);
//  llvm::BasicBlock* body = llvm::BasicBlock::Create(context_, "body");
//  llvm::BasicBlock* merge = llvm::BasicBlock::Create(context_, "merge");
//  builder_->CreateCondBr(condv, body, merge);
//
//  now->getBasicBlockList().push_back(body);
//  builder_->SetInsertPoint(body);
//  whilestmt->body()->Accept(this);
//  builder_->CreateBr(whileblock);
//  body = builder_->GetInsertBlock();
//
//  now->getBasicBlockList().push_back(merge);
//  builder_->SetInsertPoint(merge);
//  ret_ = NULL;
}

void CodeGenerator::Visit(ForStatement* forstmt) {
  llvm::Function* now = builder_->GetInsertBlock()->getParent();
  if (forstmt->init()) {
    forstmt->init()->Accept(this);
  }
  llvm::BasicBlock* forblock = llvm::BasicBlock::Create(context_, "block", now);

  builder_->SetInsertPoint(forblock);

  llvm::BasicBlock* body = llvm::BasicBlock::Create(context_, "body");
  llvm::BasicBlock* merge = llvm::BasicBlock::Create(context_, "merge");
  if (forstmt->cond()) {
    forstmt->cond()->Accept(this);
    llvm::Value* condv = IsJSTrue(ret_);
    builder_->CreateCondBr(condv, body, merge);
  } else {
    builder_->CreateBr(body);
  }

  now->getBasicBlockList().push_back(body);
  builder_->SetInsertPoint(body);
  forstmt->body()->Accept(this);
  if (forstmt->next()) {
    forstmt->next()->Accept(this);
  }
  builder_->CreateBr(forblock);
  body = builder_->GetInsertBlock();

  now->getBasicBlockList().push_back(merge);
  builder_->SetInsertPoint(merge);
  ret_ = NULL;
}

void CodeGenerator::Visit(ForInStatement* forstmt) {
}

void CodeGenerator::Visit(ContinueStatement* continuestmt) {
}

void CodeGenerator::Visit(BreakStatement* breakstmt) {
}

void CodeGenerator::Visit(ReturnStatement* returnstmt) {
}

void CodeGenerator::Visit(WithStatement* withstmt) {
}

void CodeGenerator::Visit(LabelledStatement* labelledstmt) {
}

void CodeGenerator::Visit(CaseClause* clause) {
}

void CodeGenerator::Visit(SwitchStatement* switchstmt) {
}

void CodeGenerator::Visit(ThrowStatement* throwstmt) {
}

void CodeGenerator::Visit(TryStatement* trystmt) {
}

void CodeGenerator::Visit(DebuggerStatement* debuggerstmt) {
}

void CodeGenerator::Visit(ExpressionStatement* exprstmt) {
}

void CodeGenerator::Visit(Assignment* assign) {
}

void CodeGenerator::Visit(BinaryOperation* binary) {
}

void CodeGenerator::Visit(ConditionalExpression* cond) {
}

void CodeGenerator::Visit(UnaryOperation* unary) {
}

void CodeGenerator::Visit(PostfixExpression* postfix) {
}

void CodeGenerator::Visit(StringLiteral* literal) {
  // llvm::Value* p = builder_->CreateGEP(literal->value(), Zero(), Zero());

//  llvm::GlobalVariable* v = global_variable(module_,
//                                            ArrayType::get(int16ty_,
//                                                     literal.value.length+1),
//                                      true,
//                                      llvm:);
}

void CodeGenerator::Visit(NumberLiteral* literal) {
}

void CodeGenerator::Visit(Identifier* literal) {
}

void CodeGenerator::Visit(ThisLiteral* literal) {
}

void CodeGenerator::Visit(NullLiteral* literal) {
}

void CodeGenerator::Visit(TrueLiteral* literal) {
}

void CodeGenerator::Visit(FalseLiteral* literal) {
}

void CodeGenerator::Visit(Undefined* literal) {
}

void CodeGenerator::Visit(RegExpLiteral* literal) {
}

void CodeGenerator::Visit(ArrayLiteral* literal) {
}

void CodeGenerator::Visit(ObjectLiteral* literal) {
}

void CodeGenerator::Visit(FunctionLiteral* literal) {
  std::for_each(literal->body().begin(),
                literal->body().end(),
                Acceptor<CodeGenerator>(this));
}

void CodeGenerator::Visit(PropertyAccess* prop) {
}

void CodeGenerator::Visit(FunctionCall* call) {
}

void CodeGenerator::Visit(ConstructorCall* call) {
}

llvm::Value* CodeGenerator::IsJSTrue(llvm::Value* val) {
  return IsNonZero(val);
}

llvm::Value* CodeGenerator::GetConstantStringPtr(const UnicodeString& str) {
  llvm::WeakVH val = constant_strings_[str.getBuffer()];
  return val;
}

} }  // namespace iv::core
