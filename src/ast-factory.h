#ifndef _IV_AST_FACTORY_H_
#define _IV_AST_FACTORY_H_
#include "functor.h"
#include "ast.h"
#include "alloc-inl.h"

namespace iv {
namespace core {

class AstFactory : public Space {
 public:
  AstFactory();
  inline void Clear() {
    Space::Clear();
  }
  Identifier* NewIdentifier(const UChar* buffer);
  Identifier* NewIdentifier(const char* buffer);
  Identifier* NewIdentifier(const std::vector<UChar>& buffer);
  Identifier* NewIdentifier(const std::vector<char>& buffer);
  StringLiteral* NewStringLiteral(const std::vector<UChar>& buffer);
  Directivable* NewDirectivable(const std::vector<UChar>& buffer);
  RegExpLiteral* NewRegExpLiteral(const std::vector<UChar>& buffer);
  FunctionLiteral* NewFunctionLiteral(FunctionLiteral::DeclType type);
  ArrayLiteral* NewArrayLiteral();
  ObjectLiteral* NewObjectLiteral();
  AstNode::Identifiers* NewLabels();

  inline NullLiteral* NewNullLiteral() {
    return null_instance_;
  }
  inline EmptyStatement* NewEmptyStatement() {
    return empty_statement_instance_;
  }
  inline DebuggerStatement* NewDebuggerStatement() {
    return debugger_statement_instance_;
  }
  inline ThisLiteral* NewThisLiteral() {
    return this_instance_;
  }
  inline Undefined* NewUndefined() {
    return undefined_instance_;
  }
  inline TrueLiteral* NewTrueLiteral() {
    return true_instance_;
  }
  inline FalseLiteral* NewFalseLiteral() {
    return false_instance_;
  }
 private:
  Undefined* undefined_instance_;
  EmptyStatement* empty_statement_instance_;
  DebuggerStatement* debugger_statement_instance_;
  ThisLiteral* this_instance_;
  NullLiteral* null_instance_;
  TrueLiteral* true_instance_;
  FalseLiteral* false_instance_;
};

} }  // namespace iv::core
#endif  // _IV_AST_FACTORY_H_
