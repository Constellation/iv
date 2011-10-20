#ifndef IV_AST_SERIALIZER_H_
#define IV_AST_SERIALIZER_H_
#include <cstdio>
#include <ostream>  // NOLINT
#include <sstream>
#include "detail/tuple.h"
#include "detail/cstdint.h"
#include "ast.h"
#include "ast_visitor.h"
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "string_builder.h"
#include "conversions.h"
namespace iv {
namespace core {
namespace ast {

template<typename Factory>
class AstSerializer: public AstVisitor<Factory>::const_type {
 public:

  typedef void ReturnType;

#define V(AST) typedef typename core::ast::AST<Factory> AST;
  IV_AST_NODE_LIST(V)
#undef V
#define V(XS) typedef typename core::ast::AstNode<Factory>::XS XS;
  IV_AST_LIST_LIST(V)
#undef V
#define V(S) typedef typename core::SpaceUString<Factory>::type S;
  IV_AST_STRING(V)
#undef V

  AstSerializer() : builder_() { }
  inline UString out() const {
    return builder_.Build();
  }

  void Visit(const Block* block) {
    builder_.Append("{\"type\":\"block\",\"body\":[");
    typename Statements::const_iterator it = block->body().begin();
    const typename Statements::const_iterator end = block->body().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it != end) {
        builder_.Append(',');
      }
    }
    builder_.Append("]}");
  }

  void Visit(const FunctionStatement* func) {
    builder_.Append("{\"type\":\"function_statement\",\"def\":");
    func->function()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const FunctionDeclaration* func) {
    builder_.Append("{\"type\":\"function_declaration\",\"def\":");
    func->function()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const VariableStatement* var) {
    builder_.Append("{\"type\":");
    if (var->IsConst()) {
      builder_.Append("\"const\",\"decls\":[");
    } else {
      builder_.Append("\"var\",\"decls\":[");
    }
    typename Declarations::const_iterator it = var->decls().begin();
    const typename Declarations::const_iterator end = var->decls().end();
    while (it != end) {
      const Declaration& decl = **it;
      builder_.Append("{\"type\":\"decl\",\"name\":");
      decl.name()->Accept(this);
      builder_.Append(",\"exp\":");
      if (const Maybe<const Expression> expr = decl.expr()) {
        (*expr).Accept(this);
      }
      builder_.Append('}');
      ++it;
      if (it != end) {
        builder_.Append(',');
      }
    }
    builder_.Append("]}");
  }

  void Visit(const EmptyStatement* empty) {
    builder_.Append("{\"type\":\"empty\"}");
  }

  void Visit(const IfStatement* ifstmt) {
    builder_.Append("{\"type\":\"if\",\"cond\":");
    ifstmt->cond()->Accept(this);
    builder_.Append(",\"body\":");
    ifstmt->then_statement()->Accept(this);
    if (const Maybe<const Statement> else_stmt = ifstmt->else_statement()) {
      builder_.Append(",\"else\":");
      (*else_stmt).Accept(this);
    }
    builder_.Append('}');
  }

  void Visit(const DoWhileStatement* dowhile) {
    builder_.Append("{\"type\":\"dowhile\",\"cond\":");
    dowhile->cond()->Accept(this);
    builder_.Append(",\"body\":");
    dowhile->body()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const WhileStatement* whilestmt) {
    builder_.Append("{\"type\":\"while\",\"cond\":");
    whilestmt->cond()->Accept(this);
    builder_.Append(",\"body\":");
    whilestmt->body()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const ForStatement* forstmt) {
    builder_.Append("{\"type\":\"for\"");
    if (const Maybe<const Statement> init = forstmt->init()) {
      builder_.Append(",\"init\":");
      (*init).Accept(this);
    }
    if (const Maybe<const Expression> cond = forstmt->cond()) {
      builder_.Append(",\"cond\":");
      (*cond).Accept(this);
    }
    if (const Maybe<const Expression> next = forstmt->next()) {
      builder_.Append(",\"next\":");
      (*next).Accept(this);
    }
    builder_.Append(",\"body\":");
    forstmt->body()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const ForInStatement* forstmt) {
    builder_.Append("{\"type\":\"forin\",\"each\":");
    forstmt->each()->Accept(this);
    builder_.Append(",\"enumerable\":");
    forstmt->enumerable()->Accept(this);
    builder_.Append(",\"body\":");
    forstmt->body()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const ContinueStatement* continuestmt) {
    builder_.Append("{\"type\":\"continue\"");
    if (const Maybe<const Identifier> label = continuestmt->label()) {
      builder_.Append(",\"label\":");
      (*label).Accept(this);
    }
    builder_.Append('}');
  }

  void Visit(const BreakStatement* breakstmt) {
    builder_.Append("{\"type\":\"break\"");
    if (const Maybe<const Identifier> label = breakstmt->label()) {
      builder_.Append(",\"label\":");
      (*label).Accept(this);
    }
    builder_.Append('}');
  }

  void Visit(const ReturnStatement* returnstmt) {
    builder_.Append("{\"type\":\"return\"");
    if (const Maybe<const Expression> expr = returnstmt->expr()) {
      builder_.Append(",\"exp\":");
      (*expr).Accept(this);
    }
    builder_.Append('}');
  }

  void Visit(const WithStatement* withstmt) {
    builder_.Append("{\"type\":\"with\",\"context\":");
    withstmt->context()->Accept(this);
    builder_.Append(",\"body\":");
    withstmt->body()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const LabelledStatement* labelledstmt) {
    builder_.Append("{\"type\":\"labelled\",\"label\":");
    labelledstmt->label()->Accept(this);
    builder_.Append(",\"body\":");
    labelledstmt->body()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const SwitchStatement* switchstmt) {
    builder_.Append("{\"type\":\"switch\",\"exp\":");
    switchstmt->expr()->Accept(this);
    builder_.Append(",\"clauses\":[");
    typename CaseClauses::const_iterator
        it = switchstmt->clauses().begin();
    const typename CaseClauses::const_iterator
        end = switchstmt->clauses().end();
    while (it != end) {
      const CaseClause& clause = **it;
      if (const Maybe<const Expression> expr = clause.expr()) {
        builder_.Append("{\"type\":\"case\",\"exp\":");
        (*expr).Accept(this);
      } else {
        builder_.Append("{\"type\":\"default\"");
      }
      builder_.Append(",\"body\"[");
      typename Statements::const_iterator stit = clause.body().begin();
      const typename Statements::const_iterator stend = clause.body().end();
      while (stit != stend) {
        (*stit)->Accept(this);
        ++stit;
        if (stit != stend) {
          builder_.Append(',');
        }
      }
      builder_.Append("]}");
      ++it;
      if (it != end) {
        builder_.Append(',');
      }
    }
    builder_.Append("]}");
  }

  void Visit(const ThrowStatement* throwstmt) {
    builder_.Append("{\"type\":\"throw\",\"exp\":");
    throwstmt->expr()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const TryStatement* trystmt) {
    builder_.Append("{\"type\":\"try\",\"body\":");
    trystmt->body()->Accept(this);
    if (const Maybe<const Identifier> ident = trystmt->catch_name()) {
      builder_.Append(",\"catch\":{\"type\":\"catch\",\"name\":");
      Visit(ident.Address());
      builder_.Append(",\"body\":");
      Visit(trystmt->catch_block().Address());
      builder_.Append('}');
    }
    if (const Maybe<const Block> block = trystmt->finally_block()) {
      builder_.Append(",\"finally\":{\"type\":\"finally\",\"body\":");
      Visit(block.Address());
      builder_.Append('}');
    }
    builder_.Append('}');
  }

  void Visit(const DebuggerStatement* debuggerstmt) {
    builder_.Append("{\"type\":\"debugger\"}");
  }

  void Visit(const ExpressionStatement* exprstmt) {
    builder_.Append("{\"type\":\"expstatement\",\"exp\":");
    exprstmt->expr()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const Assignment* assign) {
    builder_.Append("{\"type\":\"assign\",\"op\":\"");
    builder_.Append(Token::ToString(assign->op()));
    builder_.Append("\",\"left\":");
    assign->left()->Accept(this);
    builder_.Append(",\"right\":");
    assign->right()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const BinaryOperation* binary) {
    builder_.Append("{\"type\":\"binary\",\"op\":\"");
    builder_.Append(Token::ToString(binary->op()));
    builder_.Append("\",\"left\":");
    binary->left()->Accept(this);
    builder_.Append(",\"right\":");
    binary->right()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const ConditionalExpression* cond) {
    builder_.Append("{\"type\":\"conditional\",\"cond\":");
    cond->cond()->Accept(this);
    builder_.Append(",\"left\":");
    cond->left()->Accept(this);
    builder_.Append(",\"right\":");
    cond->right()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const UnaryOperation* unary) {
    builder_.Append("{\"type\":\"unary\",\"op\":\"");
    builder_.Append(Token::ToString(unary->op()));
    builder_.Append("\",\"exp\":");
    unary->expr()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const PostfixExpression* postfix) {
    builder_.Append("{\"type\":\"postfix\",\"op\":\"");
    builder_.Append(Token::ToString(postfix->op()));
    builder_.Append("\",\"exp\":");
    postfix->expr()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const StringLiteral* literal) {
    builder_.Append("{\"type\":\"string\",\"value\":\"");
    JSONQuote(literal->value().begin(),
              literal->value().end(),
              std::back_inserter(builder_));
    builder_.Append("\"}");
  }

  void Visit(const NumberLiteral* literal) {
    std::ostringstream sout;
    sout << literal->value();
    builder_.Append("{\"type\":\"number\",\"value\":\"");
    builder_.Append(sout.str());
    builder_.Append("\"}");
  }

  void Visit(const Identifier* literal) {
    builder_.Append("{\"type\":\"identifier\",\"value\":\"");
    builder_.Append(literal->value());
    builder_.Append("\"}");
  }

  void Visit(const ThisLiteral* literal) {
    builder_.Append("{\"type\":\"this\"}");
  }

  void Visit(const NullLiteral* literal) {
    builder_.Append("{\"type\":\"null\"}");
  }

  void Visit(const TrueLiteral* literal) {
    builder_.Append("{\"type\":\"true\"}");
  }

  void Visit(const FalseLiteral* literal) {
    builder_.Append("{\"type\":\"false\"}");
  }

  void Visit(const RegExpLiteral* literal) {
    builder_.Append("{\"type\":\"regexp\",\"value\":\"");
    JSONQuote(literal->value().begin(),
              literal->value().end(),
              std::back_inserter(builder_));
    builder_.Append("\",\"flags\":\"");
    JSONQuote(literal->flags().begin(),
              literal->flags().end(),
              std::back_inserter(builder_));
    builder_.Append("\"}");
  }

  void Visit(const ArrayLiteral* literal) {
    builder_.Append("{\"type\":\"array\",\"value\":[");
    typename MaybeExpressions::const_iterator it = literal->items().begin();
    const typename MaybeExpressions::const_iterator end =
        literal->items().end();
    bool previous_is_elision = false;
    while (it != end) {
      if ((*it)) {
        (**it).Accept(this);
        previous_is_elision = false;
      } else {
        previous_is_elision = true;
      }
      ++it;
      if (it != end) {
        builder_.Append(',');
      }
    }
    if (previous_is_elision) {
      builder_.Append(',');
    }
    builder_.Append("]}");
  }

  void Visit(const ObjectLiteral* literal) {
    using std::get;
    builder_.Append("{\"type\":\"object\",\"value\":[");
    typename ObjectLiteral::Properties::const_iterator
        it = literal->properties().begin();
    const typename ObjectLiteral::Properties::const_iterator
      end = literal->properties().end();
    while (it != end) {
      builder_.Append("{\"type\":");
      const typename ObjectLiteral::PropertyDescriptorType type(get<0>(*it));
      if (type == ObjectLiteral::DATA) {
        builder_.Append("\"data\"");
      } else if (type == ObjectLiteral::SET) {
        builder_.Append("\"setter\"");
      } else {
        builder_.Append("\"getter\"");
      }
      builder_.Append(",\"key\":");
      get<1>(*it)->Accept(this);
      builder_.Append(",\"val\":");
      get<2>(*it)->Accept(this);
      builder_.Append('}');
      ++it;
      if (it == end) {
        break;
      }
      builder_.Append(',');
    }
    builder_.Append("]}");
  }

  void Visit(const FunctionLiteral* literal) {
    builder_.Append("{\"type\":\"function\",\"name\":");
    if (const Maybe<const Identifier> name = literal->name()) {
      Visit(name.Address());
    }
    builder_.Append(",\"params\":[");
    typename Identifiers::const_iterator it = literal->params().begin();
    const typename Identifiers::const_iterator end = literal->params().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it == end) {
        break;
      }
      builder_.Append(',');
    }
    builder_.Append("],\"body\":[");
    typename Statements::const_iterator it_body = literal->body().begin();
    const typename Statements::const_iterator end_body = literal->body().end();
    while (it_body != end_body) {
      (*it_body)->Accept(this);
      ++it_body;
      if (it_body == end_body) {
        break;
      }
      builder_.Append(',');
    }
    builder_.Append("]}");
  }

  void Visit(const IndexAccess* prop) {
    builder_.Append("{\"type\":\"property\",\"target\":");
    prop->target()->Accept(this);
    builder_.Append(",\"key\":");
    prop->key()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const IdentifierAccess* prop) {
    builder_.Append("{\"type\":\"property\",\"target\":");
    prop->target()->Accept(this);
    builder_.Append(",\"key\":");
    prop->key()->Accept(this);
    builder_.Append('}');
  }

  void Visit(const FunctionCall* call) {
    builder_.Append("{\"type\":\"funcall\",\"target\":");
    call->target()->Accept(this);
    builder_.Append(",\"args\":[");
    typename Expressions::const_iterator it = call->args().begin();
    const typename Expressions::const_iterator end = call->args().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it == end) {
        break;
      }
      builder_.Append(',');
    }
    builder_.Append("]}");
  }

  void Visit(const ConstructorCall* call) {
    builder_.Append("{\"type\":\"new\",\"target\":");
    call->target()->Accept(this);
    builder_.Append(",\"args\":[");
    typename Expressions::const_iterator it = call->args().begin();
    const typename Expressions::const_iterator end = call->args().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it == end) {
        break;
      }
      builder_.Append(',');
    }
    builder_.Append("]}");
  }


  void Visit(const CaseClause* dummy) {
    UNREACHABLE();
  }

  void Visit(const Declaration* dummy) {
    UNREACHABLE();
  }

 private:
  UStringBuilder builder_;
};

} } }  // namespace iv::core::ast
#endif  // IV_AST_SERIALIZER_H_
