#ifndef _IV_AST_FACTORY_H_
#define _IV_AST_FACTORY_H_
#include "functor.h"
#include "ast.h"
#include "alloc-inl.h"
#include "regexp-creator.h"

namespace iv {
namespace core {

class BasicAstFactory : public Space {
 public:
  BasicAstFactory()
    : Space(),
      undefined_instance_(new(this)Undefined()),
      empty_statement_instance_(new(this)EmptyStatement()),
      debugger_statement_instance_(new(this)DebuggerStatement()),
      this_instance_(new(this)ThisLiteral()),
      null_instance_(new(this)NullLiteral()),
      true_instance_(new(this)TrueLiteral()),
      false_instance_(new(this)FalseLiteral()) {
  }
  virtual ~BasicAstFactory() = 0;

  inline void Clear() {
    Space::Clear();
  }

  virtual Identifier* NewIdentifier(const UChar* buffer) {
    return new (this) Identifier(buffer, this);
  }

  virtual Identifier* NewIdentifier(const char* buffer) {
    return new (this) Identifier(buffer, this);
  }

  virtual Identifier* NewIdentifier(const std::vector<UChar>& buffer) {
    return new (this) Identifier(buffer, this);
  }

  virtual Identifier* NewIdentifier(const std::vector<char>& buffer) {
    return new (this) Identifier(buffer, this);
  }

  virtual StringLiteral* NewStringLiteral(const std::vector<UChar>& buffer) {
    return new (this) StringLiteral(buffer, this);
  }

  virtual Directivable* NewDirectivable(const std::vector<UChar>& buffer) {
    return new (this) Directivable(buffer, this);
  }

  virtual RegExpLiteral* NewRegExpLiteral(const std::vector<UChar>& content,
                                          const std::vector<UChar>& flags) = 0;

  virtual FunctionLiteral* NewFunctionLiteral(FunctionLiteral::DeclType type) {
    return new (this) FunctionLiteral(type, this);
  }

  virtual ArrayLiteral* NewArrayLiteral() {
    return new (this) ArrayLiteral(this);
  }

  virtual ObjectLiteral* NewObjectLiteral() {
    return new (this) ObjectLiteral(this);
  }

  virtual AstNode::Identifiers* NewLabels() {
    typedef AstNode::Identifiers Identifiers;
    return new (New(sizeof(Identifiers)))
        Identifiers(Identifiers::allocator_type(this));
  }

  virtual NullLiteral* NewNullLiteral() {
    return null_instance_;
  }

  virtual EmptyStatement* NewEmptyStatement() {
    return empty_statement_instance_;
  }

  virtual DebuggerStatement* NewDebuggerStatement() {
    return debugger_statement_instance_;
  }

  virtual ThisLiteral* NewThisLiteral() {
    return this_instance_;
  }

  virtual Undefined* NewUndefined() {
    return undefined_instance_;
  }

  virtual TrueLiteral* NewTrueLiteral() {
    return true_instance_;
  }

  virtual FalseLiteral* NewFalseLiteral() {
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

inline BasicAstFactory::~BasicAstFactory() { }

} }  // namespace iv::core
#endif  // _IV_AST_FACTORY_H_
