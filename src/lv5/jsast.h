#ifndef _IV_LV5_JSAST_H_
#define _IV_LV5_JSAST_H_
#include <vector>
#include <cassert>
#include "uchar.h"
#include "ast.h"
#include "lv5/symbol.h"
#include "lv5/jsregexp_impl.h"

namespace iv {
namespace lv5 {
class AstFactory;
class Context;
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
  void Initialize(iv::lv5::Context* ctx) {
    regexp_ = new iv::lv5::JSRegExpImpl(
        Derived()->value(),
        Derived()->flags());
  }
  const iv::lv5::JSRegExpImpl* regexp() const {
    return regexp_;
  }
  bool IsValid() const {
    assert(regexp_);
    return regexp_->IsValid();
  }
 private:
  RegExpLiteral<iv::lv5::AstFactory>* Derived() {
    return static_cast<RegExpLiteral<iv::lv5::AstFactory>*>(this);
  }
  iv::lv5::JSRegExpImpl* regexp_;
};

} }  // namespace iv::core::ast
namespace lv5 {
class AstFactory;
#define V(AST) typedef core::ast::AST<AstFactory> AST;
  AST_NODE_LIST(V)
#undef V
#define V(XS) typedef core::ast::AstNode<AstFactory>::XS XS;
  AST_LIST_LIST(V)
#undef V
#define V(S) typedef core::SpaceUString<AstFactory>::type S;
  AST_STRING(V)
#undef V
typedef core::ast::AstVisitor<AstFactory>::const_type AstVisitor;

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSAST_H_
