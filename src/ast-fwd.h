#ifndef _IV_AST_FWD_H_
#define _IV_AST_FWD_H_
namespace iv {
namespace core {
namespace ast {

template<typename T>
class AstNode;
template<typename T>
class Identifier;
template<typename T>
class NumberLiteral;
template<typename T>
class StringLiteral;
template<typename T>
class Directivable;
template<typename T>
class RegExpLiteral;
template<typename T>
class FunctionLiteral;
template<typename T>
class ArrayLiteral;
template<typename T>
class ObjectLiteral;
template<typename T>
class NullLiteral;
template<typename T>
class EmptyStatement;
template<typename T>
class DebuggerStatement;
template<typename T>
class ThisLiteral;
template<typename T>
class Undefined;
template<typename T>
class TrueLiteral;
template<typename T>
class FalseLiteral;
template<typename T>
class FunctionStatement;
template<typename T>
class Block;
template<typename T>
class VariableStatement;
template<typename T>
class Variable;
template<typename T>
class Declaration;
template<typename T>
class IfStatement;
template<typename T>
class DoWhileStatement;
template<typename T>
class WhileStatement;
template<typename T>
class ForInStatement;
template<typename T>
class ExpressionStatement;
template<typename T>
class ForStatement;
template<typename T>
class ContinueStatement;
template<typename T>
class BreakStatement;
template<typename T>
class ReturnStatement;
template<typename T>
class WithStatement;
template<typename T>
class SwitchStatement;
template<typename T>
class CaseClause;
template<typename T>
class ThrowStatement;
template<typename T>
class TryStatement;
template<typename T>
class LabelledStatement;
template<typename T>
class BinaryOperation;
template<typename T>
class Assignment;
template<typename T>
class ConditionalExpression;
template<typename T>
class UnaryOperation;
template<typename T>
class PostfixExpression;
template<typename T>
class FunctionCall;
template<typename T>
class ConstructorCall;
template<typename T>
class IndexAccess;
template<typename T>
class IdentifierAccess;
template<typename T>
class BreakableStatement;
template<typename T>
class NamedOnlyBreakableStatement;
template<typename T>
class AnonymousBreakableStatement;
template<typename T>
class Literal;
template<typename T>
class Scope;
template<typename T>
class Expression;
template<typename T>
class Statement;
template<typename T>
class IterationStatement;
template<typename T>
class PropertyAccess;
template<typename T>
class Call;

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

} } }  // namespace iv::core::ast
#endif  // _IV_AST_FWD_H_
