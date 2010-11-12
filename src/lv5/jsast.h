#ifndef _IV_LV5_JSAST_H_
#define _IV_LV5_JSAST_H_
#include <vector>
#include "uchar.h"
#include "jsregexp-impl.h"
#include "ast.h"
#include "symbol.h"

namespace iv {
namespace lv5 {
class AstFactory;
}  // namespace iv::lv5
namespace core {
namespace ast {

template<>
class IdentifierBase<iv::lv5::AstFactory>
  : public Inherit<iv::lv5::AstFactory, kIdentifier> {
 public:
  void set_symbol(iv::lv5::Symbol sym) {
    sym_ = sym;
  }
  iv::lv5::Symbol symbol() const {
    return sym_;
  }
 private:
  iv::lv5::Symbol sym_;
};

template<>
class RegExpLiteralBase<iv::lv5::AstFactory>
  : public Inherit<iv::lv5::AstFactory, kRegExpLiteral> {
 public:
  void Initialize() {
    status_ = U_ZERO_ERROR;
    regexp_.Initialize(
        Derived()->value(),
        Derived()->flags(),
        &status_);
  }
  const iv::lv5::JSRegExpImpl& regexp() const {
    return regexp_;
  }
  bool IsValid() const {
    return status_ == U_ZERO_ERROR;
  }
 private:
  RegExpLiteral<iv::lv5::AstFactory>* Derived() {
    return static_cast<RegExpLiteral<iv::lv5::AstFactory>*>(this);
  }
  UErrorCode status_;
  iv::lv5::JSRegExpImpl regexp_;
};

} }  // namespace iv::core::ast
namespace lv5 {
class AstFactory;
#define V(AST) typedef core::ast::AST<AstFactory> AST;
  AST_NODE_LIST(V)
#undef V
#define V(X, XS) typedef core::SpaceVector<AstFactory, X *>::type XS;
  AST_LIST_LIST(V)
#undef V
#define V(S) typedef core::SpaceUString<AstFactory>::type S;
  AST_STRING(V)
#undef V
typedef core::ast::AstVisitor<AstFactory>::const_type AstVisitor;

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSAST_H_
