#ifndef IV_LV5_SPECIALIZED_AST_H_
#define IV_LV5_SPECIALIZED_AST_H_
#include <vector>
#include <cassert>
#include <iv/ast.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/regexp.h>

namespace iv {
namespace lv5 {

class AstFactory;
class Context;

namespace context {

void RegisterLiteralRegExp(Context* ctx, RegExp* reg);

} }  // namespace lv5::context
namespace core {
namespace ast {

template<>
class RegExpLiteralBase<iv::lv5::AstFactory>
  : public Inherit<iv::lv5::AstFactory, kRegExpLiteral> {
 public:
  void Initialize(iv::lv5::Context* ctx) {
    const int flags = iv::lv5::RegExp::ComputeFlags(
        Derived()->flags().begin(),
        Derived()->flags().end());
    regexp_ = new iv::lv5::RegExp(
        ctx->regexp_allocator(),
        Derived()->value(), flags);
    ctx->global_data()->RegisterLiteralRegExp(regexp_);
  }
  const iv::lv5::RegExp* regexp() const {
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
  iv::lv5::RegExp* regexp_;
};


template<>
class StatementBase<iv::lv5::AstFactory>
  : public Inherit<iv::lv5::AstFactory, kStatement> {
 public:
  virtual bool IsEffectiveStatement() const {
    return true;
  }
};

template<>
class EmptyStatementBase<iv::lv5::AstFactory>
  : public Inherit<iv::lv5::AstFactory, kEmptyStatement> {
 public:
  virtual bool IsEffectiveStatement() const {
    return false;
  }
};

template<>
class BlockBase<iv::lv5::AstFactory>
  : public Inherit<iv::lv5::AstFactory, kBlock> {
 public:

  struct FindEffectiveStatement {
    template<typename T>
    bool operator()(const T* stmt) const {
      return stmt->IsEffectiveStatement();
    }
  };

  virtual bool IsEffectiveStatement() const {
    typedef const iv::core::ast::Block<iv::lv5::AstFactory>* ConstBlockPtrType;
    return std::find_if(
        static_cast<ConstBlockPtrType>(this)->body().begin(),
        static_cast<ConstBlockPtrType>(this)->body().end(),
        FindEffectiveStatement()) !=
        static_cast<ConstBlockPtrType>(this)->body().end();
  }
};

} }  // namespace core::ast
namespace lv5 {
class AstFactory;
#define V(AST) typedef core::ast::AST<AstFactory> AST;
  IV_AST_NODE_LIST(V)
#undef V
#define V(XS) typedef core::ast::AstNode<AstFactory>::XS XS;
  IV_AST_LIST_LIST(V)
#undef V
#define V(S) typedef core::SpaceUString<AstFactory>::type S;
  IV_AST_STRING(V)
#undef V
typedef core::ast::AstVisitor<AstFactory>::const_type AstVisitor;

typedef core::ast::ExpressionVisitor<AstFactory>::const_type ExpressionVisitor;

} }  // namespace iv::lv5
#endif  // IV_LV5_SPECIALIZED_AST_H_
