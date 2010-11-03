#ifndef _IV_AST_VISITOR_H_
#define _IV_AST_VISITOR_H_
#include <tr1/type_traits>
#include "noncopyable.h"

namespace iv {
namespace core {
namespace detail {

template<bool IsConst, typename T>
struct AstVisitorTraits {
};

template<typename T>
struct AstVisitorTraits<true, T> {
  typedef typename std::tr1::add_pointer<
      typename std::tr1::add_const<T>::type>::type type;
};

template<typename T>
struct AstVisitorTraits<false, T> {
  typedef typename std::tr1::add_pointer<
      typename std::tr1::add_const<T>::type>::type type;
};

}  // namespace iv::core::detail

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

template<bool IsConst>
class BasicAstVisitor : private Noncopyable<BasicAstVisitor<IsConst> >::type {
 private:
  template<typename T>
  struct add {
    typedef typename detail::AstVisitorTraits<IsConst, T>::type type;
  };
 public:
  virtual ~BasicAstVisitor() = 0;
  virtual void Visit(typename add<Block>::type block) = 0;
  virtual void Visit(typename add<FunctionStatement>::type func) = 0;
  virtual void Visit(typename add<VariableStatement>::type var) = 0;
  virtual void Visit(typename add<Declaration>::type decl) = 0;
  virtual void Visit(typename add<EmptyStatement>::type empty) = 0;
  virtual void Visit(typename add<IfStatement>::type ifstmt) = 0;
  virtual void Visit(typename add<DoWhileStatement>::type dowhile) = 0;
  virtual void Visit(typename add<WhileStatement>::type whilestmt) = 0;
  virtual void Visit(typename add<ForStatement>::type forstmt) = 0;
  virtual void Visit(typename add<ForInStatement>::type forstmt) = 0;
  virtual void Visit(typename add<ContinueStatement>::type continuestmt) = 0;
  virtual void Visit(typename add<BreakStatement>::type breakstmt) = 0;
  virtual void Visit(typename add<ReturnStatement>::type returnstmt) = 0;
  virtual void Visit(typename add<WithStatement>::type withstmt) = 0;
  virtual void Visit(typename add<LabelledStatement>::type labelledstmt) = 0;
  virtual void Visit(typename add<CaseClause>::type clause) = 0;
  virtual void Visit(typename add<SwitchStatement>::type switchstmt) = 0;
  virtual void Visit(typename add<ThrowStatement>::type throwstmt) = 0;
  virtual void Visit(typename add<TryStatement>::type trystmt) = 0;
  virtual void Visit(typename add<DebuggerStatement>::type debuggerstmt) = 0;
  virtual void Visit(typename add<ExpressionStatement>::type exprstmt) = 0;

  virtual void Visit(typename add<Assignment>::type assign) = 0;
  virtual void Visit(typename add<BinaryOperation>::type binary) = 0;
  virtual void Visit(typename add<ConditionalExpression>::type cond) = 0;
  virtual void Visit(typename add<UnaryOperation>::type unary) = 0;
  virtual void Visit(typename add<PostfixExpression>::type postfix) = 0;

  virtual void Visit(typename add<StringLiteral>::type literal) = 0;
  virtual void Visit(typename add<NumberLiteral>::type literal) = 0;
  virtual void Visit(typename add<Identifier>::type literal) = 0;
  virtual void Visit(typename add<ThisLiteral>::type literal) = 0;
  virtual void Visit(typename add<NullLiteral>::type literal) = 0;
  virtual void Visit(typename add<TrueLiteral>::type literal) = 0;
  virtual void Visit(typename add<FalseLiteral>::type literal) = 0;
  virtual void Visit(typename add<Undefined>::type literal) = 0;
  virtual void Visit(typename add<RegExpLiteral>::type literal) = 0;
  virtual void Visit(typename add<ArrayLiteral>::type literal) = 0;
  virtual void Visit(typename add<ObjectLiteral>::type literal) = 0;
  virtual void Visit(typename add<FunctionLiteral>::type literal) = 0;

  virtual void Visit(typename add<IdentifierAccess>::type prop) = 0;
  virtual void Visit(typename add<IndexAccess>::type prop) = 0;
  virtual void Visit(typename add<FunctionCall>::type call) = 0;
  virtual void Visit(typename add<ConstructorCall>::type call) = 0;
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

template<bool IsConst>
inline BasicAstVisitor<IsConst>::~BasicAstVisitor() { }

typedef BasicAstVisitor<false> AstVisitor;
typedef BasicAstVisitor<true> ConstAstVisitor;

} }  // namespace iv::core
#endif  // _IV_AST_VISITOR_H_
