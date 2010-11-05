#ifndef _IV_LV5_JSAST_H_
#define _IV_LV5_JSAST_H_
#include "ast.h"
namespace iv {
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
