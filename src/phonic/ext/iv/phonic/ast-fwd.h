#ifndef _IV_PHONIC_AST_FWD_H_
#define _IV_PHONIC_AST_FWD_H_
#include <ruby.h>
#include <iv/ast.h>
namespace iv {
namespace phonic {

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

} }  // namespace iv::phonic
#endif  // _IV_PHONIC_AST_FWD_H_
