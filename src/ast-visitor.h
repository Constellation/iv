#ifndef _IV_AST_VISITOR_H_
#define _IV_AST_VISITOR_H_

namespace iv {
namespace core {
class AstNode;
class Statement;
class Block;
class FunctionStatement;
class VariableStatement;
class Declaration;
class EmptyStatement;
class IfStatement;
class IterationStatement;
class DoWhileStatement;
class WhileStatement;
class ForStatement;
class ForInStatement;
class ContinueStatement;
class BreakStatement;
class ReturnStatement;
class WithStatement;
class LabelledStatement;
class CaseClause;
class SwitchStatement;
class ThrowStatement;
class TryStatement;
class DebuggerStatement;
class ExpressionStatement;

class Assignment;
class BinaryOperation;
class ConditionalExpression;
class UnaryOperation;
class PostfixExpression;

class StringLiteral;
class NumberLiteral;
class Identifier;
class ThisLiteral;
class NullLiteral;
class TrueLiteral;
class FalseLiteral;
class Undefined;
class RegExpLiteral;
class ArrayLiteral;
class ObjectLiteral;
class FunctionLiteral;

class PropertyAccess;
class IdentifierAccess;
class IndexAccess;
class FunctionCall;
class ConstructorCall;

class AstVisitor {
 public:
  virtual ~AstVisitor() = 0;
  virtual void Visit(Block* block) = 0;
  virtual void Visit(FunctionStatement* func) = 0;
  virtual void Visit(VariableStatement* var) = 0;
  virtual void Visit(Declaration* decl) = 0;
  virtual void Visit(EmptyStatement* empty) = 0;
  virtual void Visit(IfStatement* ifstmt) = 0;
  virtual void Visit(DoWhileStatement* dowhile) = 0;
  virtual void Visit(WhileStatement* whilestmt) = 0;
  virtual void Visit(ForStatement* forstmt) = 0;
  virtual void Visit(ForInStatement* forstmt) = 0;
  virtual void Visit(ContinueStatement* continuestmt) = 0;
  virtual void Visit(BreakStatement* breakstmt) = 0;
  virtual void Visit(ReturnStatement* returnstmt) = 0;
  virtual void Visit(WithStatement* withstmt) = 0;
  virtual void Visit(LabelledStatement* labelledstmt) = 0;
  virtual void Visit(CaseClause* clause) = 0;
  virtual void Visit(SwitchStatement* switchstmt) = 0;
  virtual void Visit(ThrowStatement* throwstmt) = 0;
  virtual void Visit(TryStatement* trystmt) = 0;
  virtual void Visit(DebuggerStatement* debuggerstmt) = 0;
  virtual void Visit(ExpressionStatement* exprstmt) = 0;

  virtual void Visit(Assignment* assign) = 0;
  virtual void Visit(BinaryOperation* binary) = 0;
  virtual void Visit(ConditionalExpression* cond) = 0;
  virtual void Visit(UnaryOperation* unary) = 0;
  virtual void Visit(PostfixExpression* postfix) = 0;

  virtual void Visit(StringLiteral* literal) = 0;
  virtual void Visit(NumberLiteral* literal) = 0;
  virtual void Visit(Identifier* literal) = 0;
  virtual void Visit(ThisLiteral* literal) = 0;
  virtual void Visit(NullLiteral* literal) = 0;
  virtual void Visit(TrueLiteral* literal) = 0;
  virtual void Visit(FalseLiteral* literal) = 0;
  virtual void Visit(Undefined* literal) = 0;
  virtual void Visit(RegExpLiteral* literal) = 0;
  virtual void Visit(ArrayLiteral* literal) = 0;
  virtual void Visit(ObjectLiteral* literal) = 0;
  virtual void Visit(FunctionLiteral* literal) = 0;

  virtual void Visit(IdentifierAccess* prop) = 0;
  virtual void Visit(IndexAccess* prop) = 0;
  virtual void Visit(FunctionCall* call) = 0;
  virtual void Visit(ConstructorCall* call) = 0;
 protected:
  template<typename T>
  class Acceptor {
   public:
    explicit Acceptor(T* visitor) :  visitor_(visitor) { }
    template<typename U>
    void operator()(U* p) const {
      p->Accept(visitor_);
    }
   private:
    T* const visitor_;
  };

  template<typename T>
  class Visitor {
   public:
    explicit Visitor(T* visitor) :  visitor_(visitor) { }
    template<typename U>
    void operator()(U* p) const {
      visitor_->Visit(p);
    }
   private:
    T* const visitor_;
  };
};

inline AstVisitor::~AstVisitor() { }

class ConstAstVisitor {
 public:
  virtual ~ConstAstVisitor() = 0;
  virtual void Visit(const Block* block) = 0;
  virtual void Visit(const FunctionStatement* func) = 0;
  virtual void Visit(const VariableStatement* var) = 0;
  virtual void Visit(const Declaration* decl) = 0;
  virtual void Visit(const EmptyStatement* empty) = 0;
  virtual void Visit(const IfStatement* ifstmt) = 0;
  virtual void Visit(const DoWhileStatement* dowhile) = 0;
  virtual void Visit(const WhileStatement* whilestmt) = 0;
  virtual void Visit(const ForStatement* forstmt) = 0;
  virtual void Visit(const ForInStatement* forstmt) = 0;
  virtual void Visit(const ContinueStatement* continuestmt) = 0;
  virtual void Visit(const BreakStatement* breakstmt) = 0;
  virtual void Visit(const ReturnStatement* returnstmt) = 0;
  virtual void Visit(const WithStatement* withstmt) = 0;
  virtual void Visit(const LabelledStatement* labelledstmt) = 0;
  virtual void Visit(const CaseClause* clause) = 0;
  virtual void Visit(const SwitchStatement* switchstmt) = 0;
  virtual void Visit(const ThrowStatement* throwstmt) = 0;
  virtual void Visit(const TryStatement* trystmt) = 0;
  virtual void Visit(const DebuggerStatement* debuggerstmt) = 0;
  virtual void Visit(const ExpressionStatement* exprstmt) = 0;

  virtual void Visit(const Assignment* assign) = 0;
  virtual void Visit(const BinaryOperation* binary) = 0;
  virtual void Visit(const ConditionalExpression* cond) = 0;
  virtual void Visit(const UnaryOperation* unary) = 0;
  virtual void Visit(const PostfixExpression* postfix) = 0;

  virtual void Visit(const StringLiteral* literal) = 0;
  virtual void Visit(const NumberLiteral* literal) = 0;
  virtual void Visit(const Identifier* literal) = 0;
  virtual void Visit(const ThisLiteral* literal) = 0;
  virtual void Visit(const NullLiteral* literal) = 0;
  virtual void Visit(const TrueLiteral* literal) = 0;
  virtual void Visit(const FalseLiteral* literal) = 0;
  virtual void Visit(const Undefined* literal) = 0;
  virtual void Visit(const RegExpLiteral* literal) = 0;
  virtual void Visit(const ArrayLiteral* literal) = 0;
  virtual void Visit(const ObjectLiteral* literal) = 0;
  virtual void Visit(const FunctionLiteral* literal) = 0;

  virtual void Visit(const IdentifierAccess* prop) = 0;
  virtual void Visit(const IndexAccess* prop) = 0;
  virtual void Visit(const FunctionCall* call) = 0;
  virtual void Visit(const ConstructorCall* call) = 0;
 protected:
  template<typename T>
  class Acceptor {
   public:
    explicit Acceptor(T* visitor) :  visitor_(visitor) { }
    template<typename U>
    void operator()(U* p) const {
      p->Accept(visitor_);
    }
   private:
    T* const visitor_;
  };

  template<typename T>
  class Visitor {
   public:
    explicit Visitor(T* visitor) :  visitor_(visitor) { }
    template<typename U>
    void operator()(U* p) const {
      visitor_->Visit(p);
    }
   private:
    T* const visitor_;
  };
};

inline ConstAstVisitor::~ConstAstVisitor() { }

} }  // namespace iv::core
#endif  // _IV_AST_VISITOR_H_
