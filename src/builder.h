#ifndef _IV_BUILDER_H_
#define _IV_BUILDER_H_
#include <llvm/Support/IRBuilder.h>
#include "ast.h"

namespace iv {
namespace core {

typedef llvm::IRBuilder<true, llvm::TargetFolder> BuilderT;

class CodeGenerator {
 public:
  CodeGenerator() : builder_() { }
  ~CodeGenerator() { }
  llvm::Value* Generate(NumberLiteral* num) {
    return NULL;
  }
  llvm::Value* Generate(BinaryOperation* op) {
    return NULL;
  }
  llvm::Value* Generate(Expression* expr) {
    return NULL;
  }
  llvm::Value* Generate(FunctionCall* call) {
    return NULL;
  }
 private:
  BuilderT builder_;
};



} }  // namespace iv::core
#endif  // _IV_BUILDER_H_

