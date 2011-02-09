#ifndef _IV_AST_INFO_H_
#define _IV_AST_INFO_H_
#include "ast.h"
#include "space.h"
namespace iv {
namespace core {
namespace ast {
template<typename Factory>
struct AstInfo {
#define V(AST) typedef typename AST<Factory> AST;
  AST_NODE_LIST(V)
#undef V
#define V(XS) typedef typename ast::AstNode<Factory>::XS XS;
  AST_LIST_LIST(V)
#undef V
#define V(S) typedef typename SpaceUString<Factory>::type S;
  AST_STRING(V)
#undef V
};
} } }  // namespace iv::core::ast
#endif  // _IV_AST_INFO_H_
