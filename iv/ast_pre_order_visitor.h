#ifndef IV_AST_PRE_ORDER_VISITOR_H_
#define IV_AST_PRE_ORDER_VISITOR_H_
#include <iv/ast_visitor.h>
namespace iv {
namespace core {
namespace ast {

template<bool IsConst, typename Factory>
class BasicAstPreOrderVisitor : public BasicAstVisitor<IsConst, Factory> {
 private:
  // meta function
  template<typename T>
  struct add {
    typedef typename detail::AstVisitorTraits<IsConst, T>::type type;
  };

 public:
  typedef BasicAstPreOrderVisitor<IsConst, Factory> this_type;

  virtual ~BasicAstPreOrderVisitor() { };

  virtual void Visit(typename add<Block<Factory> >::type block) {
    for (typename AstNode<Factory>::Statements::const_iterator it = block->body().begin(),
         last = block->body().end(); it != last; ++it) {
      typename add<Statement<Factory> >::type stmt = *it;
      stmt->Accept(this);
    }
  }

  virtual void Visit(typename add<FunctionStatement<Factory> >::type func) {
    typename add<FunctionLiteral<Factory> >::type literal = func->function();
    Visit(literal);
  }

  virtual void Visit(typename add<FunctionDeclaration<Factory> >::type func) {
    typename add<FunctionLiteral<Factory> >::type literal = func->function();
    Visit(literal);
  }

  virtual void Visit(typename add<VariableStatement<Factory> >::type var) {
    for (typename VariableStatement<Factory>::Declarations::const_iterator it = var->decls().begin(),
         last = var->decls().end(); it != last; ++it) {
      typename add<Declaration<Factory> >::type decl = *it;
      Visit(decl);
    }
  }

  virtual void Visit(typename add<EmptyStatement<Factory> >::type empty) {
  }

  virtual void Visit(typename add<IfStatement<Factory> >::type ifstmt) {
    ifstmt->cond()->Accept(this);
    ifstmt->then_statement()->Accept(this);
    if (ifstmt->else_statement()) {
      ifstmt->else_statement().Address()->Accept(this);
    }
  }

  virtual void Visit(typename add<DoWhileStatement<Factory> >::type dowhile) {
    dowhile->body()->Accept(this);
    dowhile->cond()->Accept(this);
  }

  virtual void Visit(typename add<WhileStatement<Factory> >::type whilestmt) {
    whilestmt->cond()->Accept(this);
    whilestmt->body()->Accept(this);
  }

  virtual void Visit(typename add<ForStatement<Factory> >::type forstmt) {
    if (forstmt->init()) {
      forstmt->init().Address()->Accept(this);
    }
    if (forstmt->cond()) {
      forstmt->cond().Address()->Accept(this);
    }
    if (forstmt->next()) {
      forstmt->next().Address()->Accept(this);
    }
    forstmt->body()->Accept(this);
  }

  virtual void Visit(typename add<ForInStatement<Factory> >::type forstmt) {
    forstmt->each()->Accept(this);
    forstmt->enumerable()->Accept(this);
    forstmt->body()->Accept(this);
  }

  virtual void Visit(typename add<ContinueStatement<Factory> >::type continuestmt) {
  }

  virtual void Visit(typename add<BreakStatement<Factory> >::type breakstmt) {
  }

  virtual void Visit(typename add<ReturnStatement<Factory> >::type returnstmt) {
    if (returnstmt->expr()) {
      returnstmt->expr().Address()->Accept(this);
    }
  }

  virtual void Visit(typename add<WithStatement<Factory> >::type withstmt) {
    withstmt->context()->Accept(this);
    withstmt->body()->Accept(this);
  }

  virtual void Visit(typename add<LabelledStatement<Factory> >::type labelledstmt) {
    labelledstmt->body()->Accept(this);
  }

  virtual void Visit(typename add<SwitchStatement<Factory> >::type switchstmt) {
    switchstmt->expr()->Accept(this);
    for (typename SwitchStatement<Factory>::CaseClauses::const_iterator it = switchstmt->clauses().begin(),
         last = switchstmt->clauses().end(); it != last; ++it) {
      Visit(*it);
    }
  }

  virtual void Visit(typename add<ThrowStatement<Factory> >::type throwstmt) {
    throwstmt->expr()->Accept(this);
  }

  virtual void Visit(typename add<TryStatement<Factory> >::type trystmt) {
    trystmt->body()->Accept(this);
    if (trystmt->catch_name()) {
      Visit(trystmt->catch_block().Address());
    }
    if (trystmt->finally_block()) {
      Visit(trystmt->finally_block().Address());
    }
  }

  virtual void Visit(typename add<DebuggerStatement<Factory> >::type debuggerstmt) {
  }

  virtual void Visit(typename add<ExpressionStatement<Factory> >::type exprstmt) {
    exprstmt->expr()->Accept(this);
  }

  virtual void Visit(typename add<Assignment<Factory> >::type assign) {
    assign->left()->Accept(this);
    assign->right()->Accept(this);
  }

  virtual void Visit(typename add<BinaryOperation<Factory> >::type binary) {
    binary->left()->Accept(this);
    binary->right()->Accept(this);
  }

  virtual void Visit(typename add<ConditionalExpression<Factory> >::type cond) {
    cond->cond()->Accept(this);
    cond->left()->Accept(this);
    cond->right()->Accept(this);
  }

  virtual void Visit(typename add<UnaryOperation<Factory> >::type unary) {
    unary->expr()->Accept(this);
  }

  virtual void Visit(typename add<PostfixExpression<Factory> >::type postfix) {
    postfix->expr()->Accept(this);
  }

  virtual void Visit(typename add<Assigned<Factory> >::type assigned) {
  }

  virtual void Visit(typename add<StringLiteral<Factory> >::type literal) {
  }

  virtual void Visit(typename add<NumberLiteral<Factory> >::type literal) {
  }

  virtual void Visit(typename add<Identifier<Factory> >::type literal) {
  }

  virtual void Visit(typename add<ThisLiteral<Factory> >::type literal) {
  }

  virtual void Visit(typename add<NullLiteral<Factory> >::type literal) {
  }

  virtual void Visit(typename add<TrueLiteral<Factory> >::type literal) {
  }

  virtual void Visit(typename add<FalseLiteral<Factory> >::type literal) {
  }

  virtual void Visit(typename add<RegExpLiteral<Factory> >::type literal) {
  }

  virtual void Visit(typename add<ArrayLiteral<Factory> >::type literal) {
    for (typename AstNode<Factory>::MaybeExpressions::const_iterator it = literal->items().begin(),
         last = literal->items().end(); it != last; ++it) {
      if (*it) {
        it->Address()->Accept(this);
      }
    }
  }

  virtual void Visit(typename add<ObjectLiteral<Factory> >::type literal) {
    for (typename ObjectLiteral<Factory>::Properties::const_iterator it = literal->properties().begin(),
         last = literal->properties().end(); it != last; ++it) {
      std::get<2>(*it)->Accept(this);
    }
  }

  virtual void Visit(typename add<FunctionLiteral<Factory> >::type literal) {
    if (literal->name()) {
      literal->name().Address()->Accept(this);
    }
    for (typename FunctionLiteral<Factory>::Assigneds::const_iterator it = literal->params().begin(),
         last = literal->params().end(); it != last; ++it) {
      Visit(*it);
    }
    for (typename AstNode<Factory>::Statements::const_iterator it = literal->body().begin(),
         last = literal->body().end(); it != last; ++it) {
      typename add<Statement<Factory> >::type stmt = *it;
      stmt->Accept(this);
    }
  }

  virtual void Visit(typename add<IdentifierAccess<Factory> >::type prop) {
    prop->target()->Accept(this);
  }

  virtual void Visit(typename add<IndexAccess<Factory> >::type prop) {
    prop->target()->Accept(this);
    prop->key()->Accept(this);
  }

  virtual void Visit(typename add<FunctionCall<Factory> >::type call) {
    call->target()->Accept(this);
    for (typename AstNode<Factory>::Expressions::const_iterator it = call->args().begin(),
         last = call->args().end(); it != last; ++it) {
      (*it)->Accept(this);
    }
  }

  virtual void Visit(typename add<ConstructorCall<Factory> >::type call) {
    call->target()->Accept(this);
    for (typename AstNode<Factory>::Expressions::const_iterator it = call->args().begin(),
         last = call->args().end(); it != last; ++it) {
      (*it)->Accept(this);
    }
  }

  virtual void Visit(typename add<Declaration<Factory> >::type decl) {
    decl->name()->Accept(this);
    if (decl->expr()) {
      decl->expr().Address()->Accept(this);
    }
  }

  virtual void Visit(typename add<CaseClause<Factory> >::type clause) {
    if (clause->expr()) {
      clause->expr().Address()->Accept(this);
    }
    for (typename CaseClause<Factory>::Statements::const_iterator it = clause->body().begin(),
         last = clause->body().end(); it != last; ++it) {
      (*it)->Accept(this);
    }
  }
};

template<typename Factory>
struct AstPreOrderVisitor {
  typedef BasicAstPreOrderVisitor<false, Factory> type;
  typedef BasicAstPreOrderVisitor<true, Factory> const_type;
};

} } }  // namespace iv::core::ast
#endif  // IV_AST_PRE_ORDER_VISITOR_H_
