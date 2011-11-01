#ifndef IV_AST_FWD_H_
#define IV_AST_FWD_H_
namespace iv {
namespace core {
namespace ast {

#define IV_STATEMENT_DERIVED_NODE_LIST(V)\
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
V(FunctionDeclaration)\
V(CaseClause)

#define IV_STATEMENT_NODE_LIST(V)\
V(Statement)\
V(IterationStatement)\
V(BreakableStatement)\
V(NamedOnlyBreakableStatement)\
V(AnonymousBreakableStatement)\
IV_STATEMENT_DERIVED_NODE_LIST(V)

#define IV_LITERAL_NODE_LIST(V)\
V(Literal)\
V(Identifier)\
V(NumberLiteral)\
V(StringLiteral)\
V(RegExpLiteral)\
V(FunctionLiteral)\
V(ArrayLiteral)\
V(ObjectLiteral)\
V(NullLiteral)\
V(ThisLiteral)\
V(TrueLiteral)\
V(FalseLiteral)

#define IV_EXPRESSION_DERIVED_NODE_LIST(V)\
V(FunctionCall)\
V(ConstructorCall)\
V(BinaryOperation)\
V(Assignment)\
V(ConditionalExpression)\
V(UnaryOperation)\
V(PostfixExpression)\
V(IdentifierAccess)\
V(IndexAccess)\
IV_LITERAL_NODE_LIST(V)

#define IV_EXPRESSION_NODE_LIST(V)\
V(Expression)\
V(PropertyAccess)\
V(Call)\
IV_EXPRESSION_DERIVED_NODE_LIST(V)

#define IV_OTHER_DERIVED_NODE_LIST(V)\
V(Declaration)

#define IV_OTHER_NODE_LIST(V)\
V(AstNode)\
V(Scope)\
V(Variable)\
V(IdentifierKey)\
IV_OTHER_DERIVED_NODE_LIST(V)

#define IV_AST_DERIVED_NODE_LIST(V)\
IV_STATEMENT_DERIVED_NODE_LIST(V)\
IV_EXPRESSION_DERIVED_NODE_LIST(V)\
IV_OTHER_DERIVED_NODE_LIST(V)

#define IV_AST_NODE_LIST(V)\
IV_OTHER_NODE_LIST(V)\
IV_STATEMENT_NODE_LIST(V)\
IV_EXPRESSION_NODE_LIST(V)

#define IV_AST_LIST_LIST(V)\
V(Identifiers)\
V(Declarations)\
V(Expressions)\
V(Statements)\
V(CaseClauses)\
V(MaybeExpressions)

#define IV_AST_STRING(V) V(SpaceUString)

#define V(AST)\
template<typename Factory>\
class AST;
IV_AST_NODE_LIST(V)
#undef V

} } }  // namespace iv::core::ast
#endif  // IV_AST_FWD_H_
