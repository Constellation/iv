#ifndef _IV_CODEGEN_H_
#define _IV_CODEGEN_H_
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/ModuleProvider.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TypeBuilder.h>
#include <llvm/Support/TargetFolder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/DenseMap.h>
#include <stack>
#include <tr1/unordered_map>
#include "ast.h"
#include "noncopyable.h"
namespace iv {
namespace core {

class CodeGenerator
    : public AstVisitor,
      private Noncopyable<CodeGenerator>::type {
 public:
  friend std::ostream& operator<<(std::ostream& os, const CodeGenerator& out);

  typedef llvm::Value* ReturnType;
  typedef llvm::IRBuilder<true, llvm::TargetFolder> Builder;
  typedef std::stack<llvm::Value*> ValueStack;
  CodeGenerator();
  ~CodeGenerator();

  void Visit(Block* block);
  void Visit(FunctionStatement* func);
  void Visit(VariableStatement* var);
  void Visit(Declaration* decl);
  void Visit(EmptyStatement* empty);
  void Visit(IfStatement* ifstmt);
  void Visit(DoWhileStatement* dowhile);
  void Visit(WhileStatement* whilestmt);
  void Visit(ForStatement* forstmt);
  void Visit(ForInStatement* forstmt);
  void Visit(ContinueStatement* continuestmt);
  void Visit(BreakStatement* breakstmt);
  void Visit(ReturnStatement* returnstmt);
  void Visit(WithStatement* withstmt);
  void Visit(LabelledStatement* labelledstmt);
  void Visit(CaseClause* clause);
  void Visit(SwitchStatement* switchstmt);
  void Visit(ThrowStatement* throwstmt);
  void Visit(TryStatement* trystmt);
  void Visit(DebuggerStatement* debuggerstmt);
  void Visit(ExpressionStatement* exprstmt);

  void Visit(Assignment* assign);
  void Visit(BinaryOperation* binary);
  void Visit(ConditionalExpression* cond);
  void Visit(UnaryOperation* unary);
  void Visit(PostfixExpression* postfix);

  void Visit(StringLiteral* literal);
  void Visit(NumberLiteral* literal);
  void Visit(Identifier* literal);
  void Visit(ThisLiteral* literal);
  void Visit(NullLiteral* literal);
  void Visit(TrueLiteral* literal);
  void Visit(FalseLiteral* literal);
  void Visit(Undefined* literal);
  void Visit(RegExpLiteral* literal);
  void Visit(ArrayLiteral* literal);
  void Visit(ObjectLiteral* literal);
  void Visit(FunctionLiteral* literal);
  void Visit(PropertyAccess* prop);
  void Visit(FunctionCall* call);
  void Visit(ConstructorCall* call);

  void Remove(llvm::Function* func) {
    engine_->getPointerToFunction(func);
    engine_->freeMachineCodeForFunction(func);
    func->replaceAllUsesWith(llvm::UndefValue::get(func->getType()));
    func->eraseFromParent();
  }
  inline const llvm::Type* GetType(const char* str) {
    return module_->getTypeByName(str);
  }
  inline llvm::Value* IsNull(llvm::Value* val) {
    return builder_->CreateICmpEQ(val,
                                  llvm::Constant::getNullValue(val->getType()));
  }
  inline llvm::Value* IsNonZero(llvm::Value* val) {
    return builder_->CreateICmpNE(val,
                                  llvm::Constant::getNullValue(val->getType()));
  }
  inline llvm::Value* IsNegative(llvm::Value* val) {
    return builder_->CreateICmpSLT(
        val, llvm::ConstantInt::getSigned(val->getType(), 0));
  }
  inline llvm::Value* IsPositive(llvm::Value* val) {
    return builder_->CreateICmpSGT(
        val, llvm::ConstantInt::getSigned(val->getType(), 0));
  }
  llvm::Value* IsJSTrue(llvm::Value* val);

  // CreateCall methods
  llvm::CallInst *CreateCall(llvm::Value* callee, const char *name = "") {
    llvm::CallInst* call = builder_->CreateCall(callee, name);
    return call;
  }
  llvm::CallInst *CreateCall(llvm::Value* callee,
                             llvm::Value* arg1, const char *name = "") {
    llvm::CallInst* call = builder_->CreateCall(callee, arg1, name);
    return call;
  }
  llvm::CallInst *CreateCall(llvm::Value* callee,
                             llvm::Value* arg1,
                             llvm::Value* arg2, const char *name = "") {
    llvm::CallInst* call = builder_->CreateCall2(callee, arg1, arg2, name);
    return call;
  }
  llvm::CallInst *CreateCall(llvm::Value* callee,
                             llvm::Value* arg1,
                             llvm::Value* arg2,
                             llvm::Value* arg3, const char *name = "") {
    llvm::CallInst* call = builder_->CreateCall3(callee,
                                                 arg1, arg2, arg3, name);
    return call;
  }
  llvm::CallInst *CreateCall(llvm::Value* callee,
                             llvm::Value* arg1,
                             llvm::Value* arg2,
                             llvm::Value* arg3,
                             llvm::Value* arg4, const char *name = "") {
    llvm::CallInst* call = builder_->CreateCall4(callee,
                                                 arg1, arg2, arg3, arg4, name);
    return call;
  }
  template<typename Iterator>
  llvm::CallInst *CreateCall(llvm::Value* callee,
                             Iterator begin,
                             Iterator end, const char *name = "") {
    llvm::CallInst* call = builder_->CreateCall(callee, begin, end, name);
    return call;
  }

  template<typename FunctionType>
  llvm::Function* GetGlobalFunction(const std::string& name) {
    return llvm::cast<llvm::Function>(
        module_->getOrInsertFunction(
            name, llvm::TypeBuilder<FunctionType, false>::get(context_)));
  }

  // getters
  inline llvm::LLVMContext& context() const {
    return context_;
  }
  inline llvm::Module* module() const {
    return module_;
  }
  inline Builder* builder() const {
    return builder_;
  }
  inline llvm::ExecutionEngine* engine() const {
    return engine_;
  }

  // Types
  inline const llvm::Type* DoubleTy() const {
    return doublety_;
  }
  inline const llvm::Type* Int32Ty() const {
    return int32ty_;
  }
  inline const llvm::Type* Int1Ty() const {
    return int1ty_;
  }

  // simple generator
  inline llvm::Value* Bool(int i) const {
    return llvm::ConstantInt::get(int1ty_, i);
  }
  inline llvm::Value* True() const {
    return Bool(1);
  }
  inline llvm::Value* False() const {
    return Bool(0);
  }
  inline llvm::Value* Double(double d) const {
    return llvm::ConstantFP::get(doublety_, d);
  }
  inline llvm::Value* UInt32(int i) const {
    return llvm::ConstantInt::get(int32ty_, i);
  }
  inline llvm::Value* Int32(int i) const {
    return llvm::ConstantInt::get(int32ty_, i, true);
  }
  inline llvm::Value* Zero() const {
    return UInt32(0);
  }

  llvm::Value* GetConstantStringPtr(const UnicodeString& str);

  // stack functions
  inline void Push(llvm::Value* val) {
    stack_.push(val);
  }

  inline llvm::Value* Pop() {
    llvm::Value* val = stack_.top();
    stack_.pop();
    return val;
  }

 private:
  void Initialize();

  llvm::LLVMContext& context_;
  Builder* builder_;
  std::string error_;
  llvm::Module* module_;
  llvm::ModuleProvider* provider_;
  llvm::ExecutionEngine* engine_;
  const llvm::Type* int1ty_;
  const llvm::Type* int8ty_;
  const llvm::Type* int16ty_;
  const llvm::Type* int32ty_;
  const llvm::Type* int64ty_;
  const llvm::Type* doublety_;
  const llvm::Type* ptr8ty_;
  const llvm::Type* ptr32ty_;
  const llvm::Type* ptr64ty_;
  const llvm::Type* ptrptrty_;
  const llvm::Type* pointersizety_;

  llvm::Value* ret_;
  llvm::DenseMap<const uc16*, llvm::WeakVH> constant_strings_;
  ValueStack stack_;
};

} }  // namespace iv::core
#endif  // _IV_CODEGEN_H_

