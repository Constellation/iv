#ifndef _IV_AST_FWD_H_
#define _IV_AST_FWD_H_
namespace iv {
namespace core {
namespace ast {

#define STATEMENT_NODE_LIST(V)\
V(Statement)\
V(EmptyStatement)\
V(DebuggerStatement)\
V(FunctionStatement)\
V(Block)\
V(VariableStatement)\
V(IfStatement)\
V(DoWhileStatement)\
V(WhileStatement)\
V(ForInStatement)\
V(ExpressionStatement)\
V(ForStatement)\
V(ContinueStatement)\
V(BreakStatement)\
V(ReturnStatement)\
V(WithStatement)\
V(SwitchStatement)\
V(ThrowStatement)\
V(TryStatement)\
V(LabelledStatement)\
V(BreakableStatement)\
V(NamedOnlyBreakableStatement)\
V(AnonymousBreakableStatement)\
V(IterationStatement)

#define LITERAL_NODE_LIST(V)\
V(Literal)\
V(Identifier)\
V(NumberLiteral)\
V(StringLiteral)\
V(Directivable)\
V(RegExpLiteral)\
V(FunctionLiteral)\
V(ArrayLiteral)\
V(ObjectLiteral)\
V(NullLiteral)\
V(ThisLiteral)\
V(Undefined)\
V(TrueLiteral)\
V(FalseLiteral)

#define EXPRESSION_NODE_LIST(V)\
V(Expression)\
V(IndexAccess)\
V(PropertyAccess)\
V(Call)\
V(FunctionCall)\
V(ConstructorCall)\
V(BinaryOperation)\
V(Assignment)\
V(ConditionalExpression)\
V(UnaryOperation)\
V(PostfixExpression)\
V(IdentifierAccess)\
LITERAL_NODE_LIST(V)

#define OTHER_NODE_LIST(V)\
V(AstNode)\
V(Variable)\
V(Declaration)\
V(IdentifierKey)\
V(Scope)\
V(CaseClause)

#define AST_NODE_LIST(V)\
OTHER_NODE_LIST(V)\
STATEMENT_NODE_LIST(V)\
EXPRESSION_NODE_LIST(V)

#define AST_LIST_LIST(V)\
V(Identifier, Identifiers)\
V(Declaration, Declarations)\
V(Expression, Expressions)\
V(Statement, Statements)\
V(CaseClause, CaseClauses)\

#define AST_STRING(V) V(SpaceUString)

#define V(AST)\
template<typename Factory>\
class AST;
AST_NODE_LIST(V)
#undef V
} } }  // namespace iv::core::ast
#endif  // _IV_AST_FWD_H_
