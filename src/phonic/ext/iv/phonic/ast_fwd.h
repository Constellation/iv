#ifndef _IV_PHONIC_AST_FWD_H_
#define _IV_PHONIC_AST_FWD_H_
#include <iv/ast.h>

namespace iv {
namespace phonic {

class AstFactory;

}  // namespace iv::phonic

namespace core {
namespace ast {

template<>
class AstNodeBase<phonic::AstFactory>
  : public Inherit<phonic::AstFactory, kAstNode> {
 public:
  void Location(std::size_t begin, std::size_t end) {
    begin_ = begin;
    end_ = end;
  }
  std::size_t begin_position() const {
    return begin_;
  }
  std::size_t end_position() const {
    return end_;
  }
 private:
  std::size_t begin_;
  std::size_t end_;
};

template<>
class IdentifierBase<phonic::AstFactory>
  : public Inherit<phonic::AstFactory, kIdentifier> {
 public:
  void set_type(core::Token::Type type) {
    type_ = type;
  }
  core::Token::Type type() const {
    return type_;
  }
 private:
  core::Token::Type type_;
};

} }  // namespace iv::core::ast

namespace phonic {

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

} }  // namespace iv::phonic
#endif  // _IV_PHONIC_AST_FWD_H_
