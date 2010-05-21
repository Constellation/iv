#ifndef _IV_AST_FACTORY_H_
#define _IV_AST_FACTORY_H_
#include <vector>
#include "functor.h"
#include "ast.h"
#include "alloc.h"

namespace iv {
namespace core {

class AstFactory : public Space {
 public:
  AstFactory();
  ~AstFactory();

  void Register(AstNode* node); Identifier* NewIdentifier(const UChar* buffer);
  Identifier* NewIdentifier(const char* buffer);
  StringLiteral* NewStringLiteral(const UChar* buffer);
  RegExpLiteral* NewRegExpLiteral(const UChar* buffer);
  FunctionLiteral* NewFunctionLiteral(FunctionLiteral::Type type);
  ArrayLiteral* NewArrayLiteral();
  ObjectLiteral* NewObjectLiteral();

  inline NullLiteral* NewNullLiteral() {
    return &null_instance_;
  }
  inline EmptyStatement* NewEmptyStatement() {
    return &empty_statement_instance_;
  }
  inline DebuggerStatement* NewDebuggerStatement() {
    return &debugger_statement_instance_;
  }
  inline ThisLiteral* NewThisLiteral() {
    return &this_instance_;
  }
  inline Undefined* NewUndefined() {
    return &undefined_instance_;
  }
 private:
  Undefined undefined_instance_;
  EmptyStatement empty_statement_instance_;
  DebuggerStatement debugger_statement_instance_;
  ThisLiteral this_instance_;
  NullLiteral null_instance_;
  std::vector<AstNode*> registered_;
};

} }  // namespace iv::core
#endif  // _IV_AST_FACTORY_H_

