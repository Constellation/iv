#ifndef IV_AST_INFO_H_
#define IV_AST_INFO_H_
#include "ast.h"
#include "space.h"
namespace iv {
namespace core {
namespace ast {
template<typename Factory>
struct AstInfo {
#define V(AST) typedef typename AST<Factory> AST;
  IV_AST_NODE_LIST(V)
#undef V
#define V(XS) typedef typename ast::AstNode<Factory>::XS XS;
  IV_AST_LIST_LIST(V)
#undef V
#define V(S) typedef typename SpaceUString<Factory>::type S;
  IV_AST_STRING(V)
#undef V
};
} } }  // namespace iv::core::ast
#endif  // IV_AST_INFO_H_
