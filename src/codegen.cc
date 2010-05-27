#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include "codegen.h"

namespace iv {
namespace core {

void CodeGenerator::Visit(Block* block) {
}

void CodeGenerator::Visit(FunctionStatement* func) {
}

void CodeGenerator::Visit(VariableStatement* var) {
}

void CodeGenerator::Visit(Declaration* decl) {
}

void CodeGenerator::Visit(EmptyStatement* empty) {
}

void CodeGenerator::Visit(IfStatement* ifstmt) {
}

void CodeGenerator::Visit(DoWhileStatement* dowhile) {
}

void CodeGenerator::Visit(WhileStatement* whilestmt) {
}

void CodeGenerator::Visit(ForStatement* forstmt) {
}

void CodeGenerator::Visit(ForInStatement* forstmt) {
}

void CodeGenerator::Visit(ContinueStatement* continuestmt) {
}

void CodeGenerator::Visit(BreakStatement* breakstmt) {
}

void CodeGenerator::Visit(ReturnStatement* returnstmt) {
}

void CodeGenerator::Visit(WithStatement* withstmt) {
}

void CodeGenerator::Visit(LabelledStatement* labelledstmt) {
}

void CodeGenerator::Visit(CaseClause* clause) {
}

void CodeGenerator::Visit(SwitchStatement* switchstmt) {
}

void CodeGenerator::Visit(ThrowStatement* throwstmt) {
}

void CodeGenerator::Visit(TryStatement* trystmt) {
}

void CodeGenerator::Visit(DebuggerStatement* debuggerstmt) {
}

void CodeGenerator::Visit(ExpressionStatement* exprstmt) {
}

void CodeGenerator::Visit(Assignment* assign) {
}

void CodeGenerator::Visit(BinaryOperation* binary) {
}

void CodeGenerator::Visit(ConditionalExpression* cond) {
}

void CodeGenerator::Visit(UnaryOperation* unary) {
}

void CodeGenerator::Visit(PostfixExpression* postfix) {
}

void CodeGenerator::Visit(StringLiteral* literal) {
}

void CodeGenerator::Visit(NumberLiteral* literal) {
}

void CodeGenerator::Visit(Identifier* literal) {
}

void CodeGenerator::Visit(ThisLiteral* literal) {
}

void CodeGenerator::Visit(NullLiteral* literal) {
}

void CodeGenerator::Visit(TrueLiteral* literal) {
}

void CodeGenerator::Visit(FalseLiteral* literal) {
}

void CodeGenerator::Visit(Undefined* literal) {
}

void CodeGenerator::Visit(RegExpLiteral* literal) {
}

void CodeGenerator::Visit(ArrayLiteral* literal) {
}

void CodeGenerator::Visit(ObjectLiteral* literal) {
}

void CodeGenerator::Visit(FunctionLiteral* literal) {
}

void CodeGenerator::Visit(PropertyAccess* prop) {
}

void CodeGenerator::Visit(FunctionCall* call) {
}

void CodeGenerator::Visit(ConstructorCall* call) {
}

} }  // namespace iv::core

