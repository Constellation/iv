#ifndef _IV_AST_SERIALIZER_H_
#define _IV_AST_SERIALIZER_H_
#include <cstdio>
#include <ostream>  // NOLINT
#include <sstream>
#include <tr1/tuple>
#include "uchar.h"
#include "ast.h"
#include "ast_visitor.h"
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
namespace iv {
namespace core {
namespace ast {

template<typename Factory>
class AstSerializer: public AstVisitor<Factory>::const_type {
 public:

  typedef void ReturnType;

#define V(AST) typedef typename core::ast::AST<Factory> AST;
  AST_NODE_LIST(V)
#undef V
#define V(XS) typedef typename core::ast::AstNode<Factory>::XS XS;
  AST_LIST_LIST(V)
#undef V
#define V(S) typedef typename core::SpaceUString<Factory>::type S;
  AST_STRING(V)
#undef V

  AstSerializer() : out_() { }
  inline const UString& out() const {
    return out_;
  }

  void Append(const StringPiece& str) {
    out_.append(str.begin(), str.end());
  }

  void Append(const UStringPiece& str) {
    out_.append(str.begin(), str.end());
  }

  void Append(uc16 c) {
    out_.push_back(c);
  }

  void Append(char c) {
    out_.push_back(c);
  }

  void Visit(const Block* block) {
    Append("{\"type\":\"block\",\"body\":[");
    typename Statements::const_iterator it = block->body().begin();
    const typename Statements::const_iterator end = block->body().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it != end) {
        Append(',');
      }
    }
    Append("]}");
  }

  void Visit(const FunctionStatement* func) {
    Append("{\"type\":\"function_statement\",\"def\":");
    func->function()->Accept(this);
    Append('}');
  }

  void Visit(const FunctionDeclaration* func) {
    Append("{\"type\":\"function_declaration\",\"def\":");
    func->function()->Accept(this);
    Append('}');
  }

  void Visit(const VariableStatement* var) {
    Append("{\"type\":");
    if (var->IsConst()) {
      Append("\"const\",\"decls\":[");
    } else {
      Append("\"var\",\"decls\":[");
    }
    typename Declarations::const_iterator it = var->decls().begin();
    const typename Declarations::const_iterator end = var->decls().end();
    while (it != end) {
      const Declaration& decl = **it;
      Append("{\"type\":\"decl\",\"name\":");
      decl.name()->Accept(this);
      Append(",\"exp\":");
      if (const Maybe<const Expression> expr = decl.expr()) {
        (*expr).Accept(this);
      }
      Append('}');
      ++it;
      if (it != end) {
        Append(',');
      }
    }
    Append("]}");
  }

  void Visit(const EmptyStatement* empty) {
    Append("{\"type\":\"empty\"}");
  }

  void Visit(const IfStatement* ifstmt) {
    Append("{\"type\":\"if\",\"cond\":");
    ifstmt->cond()->Accept(this);
    Append(",\"body\":");
    ifstmt->then_statement()->Accept(this);
    if (const Maybe<const Statement> else_stmt = ifstmt->else_statement()) {
      Append(",\"else\":");
      (*else_stmt).Accept(this);
    }
    Append('}');
  }

  void Visit(const DoWhileStatement* dowhile) {
    Append("{\"type\":\"dowhile\",\"cond\":");
    dowhile->cond()->Accept(this);
    Append(",\"body\":");
    dowhile->body()->Accept(this);
    Append('}');
  }

  void Visit(const WhileStatement* whilestmt) {
    Append("{\"type\":\"while\",\"cond\":");
    whilestmt->cond()->Accept(this);
    Append(",\"body\":");
    whilestmt->body()->Accept(this);
    Append('}');
  }

  void Visit(const ForStatement* forstmt) {
    Append("{\"type\":\"for\"");
    if (const Maybe<const Statement> init = forstmt->init()) {
      Append(",\"init\":");
      (*init).Accept(this);
    }
    if (const Maybe<const Expression> cond = forstmt->cond()) {
      Append(",\"cond\":");
      (*cond).Accept(this);
    }
    if (const Maybe<const Statement> next = forstmt->next()) {
      Append(",\"next\":");
      (*next).Accept(this);
    }
    Append(",\"body\":");
    forstmt->body()->Accept(this);
    Append('}');
  }

  void Visit(const ForInStatement* forstmt) {
    Append("{\"type\":\"forin\",\"each\":");
    forstmt->each()->Accept(this);
    Append(",\"enumerable\":");
    forstmt->enumerable()->Accept(this);
    Append(",\"body\":");
    forstmt->body()->Accept(this);
    Append('}');
  }

  void Visit(const ContinueStatement* continuestmt) {
    Append("{\"type\":\"continue\"");
    if (const Maybe<const Identifier> label = continuestmt->label()) {
      Append(",\"label\":");
      (*label).Accept(this);
    }
    Append('}');
  }

  void Visit(const BreakStatement* breakstmt) {
    Append("{\"type\":\"break\"");
    if (const Maybe<const Identifier> label = breakstmt->label()) {
      Append(",\"label\":");
      (*label).Accept(this);
    }
    Append('}');
  }

  void Visit(const ReturnStatement* returnstmt) {
    Append("{\"type\":\"return\"");
    if (const Maybe<const Expression> expr = returnstmt->expr()) {
      Append(",\"exp\":");
      (*expr).Accept(this);
    }
    Append('}');
  }

  void Visit(const WithStatement* withstmt) {
    Append("{\"type\":\"with\",\"context\":");
    withstmt->context()->Accept(this);
    Append(",\"body\":");
    withstmt->body()->Accept(this);
    Append('}');
  }

  void Visit(const LabelledStatement* labelledstmt) {
    Append("{\"type\":\"labelled\",\"label\":");
    labelledstmt->label()->Accept(this);
    Append(",\"body\":");
    labelledstmt->body()->Accept(this);
    Append('}');
  }

  void Visit(const SwitchStatement* switchstmt) {
    Append("{\"type\":\"switch\",\"exp\":");
    switchstmt->expr()->Accept(this);
    Append(",\"clauses\":[");
    typename CaseClauses::const_iterator
        it = switchstmt->clauses().begin();
    const typename CaseClauses::const_iterator
        end = switchstmt->clauses().end();
    while (it != end) {
      const CaseClause& clause = **it;
      if (const Maybe<const Expression> expr = clause.expr()) {
        Append("{\"type\":\"case\",\"exp\":");
        (*expr).Accept(this);
      } else {
        Append("{\"type\":\"default\"");
      }
      Append(",\"body\"[");
      typename Statements::const_iterator stit = clause.body().begin();
      const typename Statements::const_iterator stend = clause.body().end();
      while (stit != stend) {
        (*stit)->Accept(this);
        ++stit;
        if (stit != stend) {
          Append(',');
        }
      }
      Append("]}");
      ++it;
      if (it != end) {
        Append(',');
      }
    }
    Append("]}");
  }

  void Visit(const ThrowStatement* throwstmt) {
    Append("{\"type\":\"throw\",\"exp\":");
    throwstmt->expr()->Accept(this);
    Append('}');
  }

  void Visit(const TryStatement* trystmt) {
    Append("{\"type\":\"try\",\"body\":");
    trystmt->body()->Accept(this);
    if (const Maybe<const Identifier> ident = trystmt->catch_name()) {
      Append(",\"catch\":{\"type\":\"catch\",\"name\":");
      Visit(ident.Address());
      Append(",\"body\":");
      Visit(trystmt->catch_block().Address());
      Append('}');
    }
    if (const Maybe<const Block> block = trystmt->finally_block()) {
      Append(",\"finally\":{\"type\":\"finally\",\"body\":");
      Visit(block.Address());
      Append('}');
    }
    Append('}');
  }

  void Visit(const DebuggerStatement* debuggerstmt) {
    Append("{\"type\":\"debugger\"}");
  }

  void Visit(const ExpressionStatement* exprstmt) {
    Append("{\"type\":\"expstatement\",\"exp\":");
    exprstmt->expr()->Accept(this);
    Append('}');
  }

  void Visit(const Assignment* assign) {
    Append("{\"type\":\"assign\",\"op\":\"");
    Append(Token::ToString(assign->op()));
    Append("\",\"left\":");
    assign->left()->Accept(this);
    Append(",\"right\":");
    assign->right()->Accept(this);
    Append('}');
  }

  void Visit(const BinaryOperation* binary) {
    Append("{\"type\":\"binary\",\"op\":\"");
    Append(Token::ToString(binary->op()));
    Append("\",\"left\":");
    binary->left()->Accept(this);
    Append(",\"right\":");
    binary->right()->Accept(this);
    Append('}');
  }

  void Visit(const ConditionalExpression* cond) {
    Append("{\"type\":\"conditional\",\"cond\":");
    cond->cond()->Accept(this);
    Append(",\"left\":");
    cond->left()->Accept(this);
    Append(",\"right\":");
    cond->right()->Accept(this);
    Append('}');
  }

  void Visit(const UnaryOperation* unary) {
    Append("{\"type\":\"unary\",\"op\":\"");
    Append(Token::ToString(unary->op()));
    Append("\",\"exp\":");
    unary->expr()->Accept(this);
    Append('}');
  }

  void Visit(const PostfixExpression* postfix) {
    Append("{\"type\":\"postfix\",\"op\":\"");
    Append(Token::ToString(postfix->op()));
    Append("\",\"exp\":");
    postfix->expr()->Accept(this);
    Append('}');
  }

  void Visit(const StringLiteral* literal) {
    Append("{\"type\":\"string\",\"value\":\"");
    DecodeString(literal->value().begin(), literal->value().end());
    Append("\"}");
  }

  void Visit(const NumberLiteral* literal) {
    std::ostringstream sout;
    sout << literal->value();
    Append("{\"type\":\"number\",\"value\":\"");
    Append(sout.str());
    Append("\"}");
  }

  void Visit(const Identifier* literal) {
    Append("{\"type\":\"identifier\",\"value\":\"");
    Append(literal->value());
    Append("\"}");
  }

  void Visit(const ThisLiteral* literal) {
    Append("{\"type\":\"this\"}");
  }

  void Visit(const NullLiteral* literal) {
    Append("{\"type\":\"null\"}");
  }

  void Visit(const TrueLiteral* literal) {
    Append("{\"type\":\"true\"}");
  }

  void Visit(const FalseLiteral* literal) {
    Append("{\"type\":\"false\"}");
  }

  void Visit(const RegExpLiteral* literal) {
    Append("{\"type\":\"regexp\",\"value\":\"");
    DecodeString(literal->value().begin(), literal->value().end());
    Append("\",\"flags\":\"");
    Append(literal->flags());
    Append("\"}");
  }

  void Visit(const ArrayLiteral* literal) {
    Append("{\"type\":\"array\",\"value\":[");
    typename MaybeExpressions::const_iterator it = literal->items().begin();
    const typename MaybeExpressions::const_iterator end = literal->items().end();
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
        Append(',');
      }
    }
    if (previous_is_elision) {
      Append(',');
    }
    Append("]}");
  }

  void Visit(const ObjectLiteral* literal) {
    using std::tr1::get;
    Append("{\"type\":\"object\",\"value\":[");
    typename ObjectLiteral::Properties::const_iterator
        it = literal->properties().begin();
    const typename ObjectLiteral::Properties::const_iterator
      end = literal->properties().end();
    while (it != end) {
      Append("{\"type\":");
      const typename ObjectLiteral::PropertyDescriptorType type(get<0>(*it));
      if (type == ObjectLiteral::DATA) {
        Append("\"data\"");
      } else if (type == ObjectLiteral::SET) {
        Append("\"setter\"");
      } else {
        Append("\"getter\"");
      }
      Append(",\"key\":");
      get<1>(*it)->Accept(this);
      Append(",\"val\":");
      get<2>(*it)->Accept(this);
      Append('}');
      ++it;
      if (it == end) {
        break;
      }
      Append(',');
    }
    Append("]}");
  }

  void Visit(const FunctionLiteral* literal) {
    Append("{\"type\":\"function\",\"name\":");
    if (const Maybe<const Identifier> name = literal->name()) {
      Visit(name.Address());
    }
    Append(",\"params\":[");
    typename Identifiers::const_iterator it = literal->params().begin();
    const typename Identifiers::const_iterator end = literal->params().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it == end) {
        break;
      }
      Append(',');
    }
    Append("],\"body\":[");
    typename Statements::const_iterator it_body = literal->body().begin();
    const typename Statements::const_iterator end_body = literal->body().end();
    while (it_body != end_body) {
      (*it_body)->Accept(this);
      ++it_body;
      if (it_body == end_body) {
        break;
      }
      Append(',');
    }
    Append("]}");
  }

  void Visit(const IndexAccess* prop) {
    Append("{\"type\":\"property\",\"target\":");
    prop->target()->Accept(this);
    Append(",\"key\":");
    prop->key()->Accept(this);
    Append('}');
  }

  void Visit(const IdentifierAccess* prop) {
    Append("{\"type\":\"property\",\"target\":");
    prop->target()->Accept(this);
    Append(",\"key\":");
    prop->key()->Accept(this);
    Append('}');
  }

  void Visit(const FunctionCall* call) {
    Append("{\"type\":\"funcall\",\"target\":");
    call->target()->Accept(this);
    Append(",\"args\":[");
    typename Expressions::const_iterator it = call->args().begin();
    const typename Expressions::const_iterator end = call->args().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it == end) {
        break;
      }
      Append(',');
    }
    Append("]}");
  }

  void Visit(const ConstructorCall* call) {
    Append("{\"type\":\"new\",\"target\":");
    call->target()->Accept(this);
    Append(",\"args\":[");
    typename Expressions::const_iterator it = call->args().begin();
    const typename Expressions::const_iterator end = call->args().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it == end) {
        break;
      }
      Append(',');
    }
    Append("]}");
  }


  void Visit(const CaseClause* dummy) {
    UNREACHABLE();
  }

  void Visit(const Declaration* dummy) {
    UNREACHABLE();
  }

 private:
  template <class Iter>
  void DecodeString(Iter it, const Iter last) {
    char buf[5];
    for (;it != last; ++it) {
      const uc16 val = *it;
      switch (val) {
        case '"':
          Append("\\\"");
          break;

        case '\\':
          Append("\\\\");
          break;

        case '/':
          Append("\\/");
          break;

        case '\b':
          Append("\\b");
          break;

        case '\f':
          Append("\\f");
          break;

        case '\n':
          Append("\\n");
          break;

        case '\r':
          Append("\\r");
          break;

        case '\t':
          Append("\\t");
          break;

        case '\x0B':  // \v
          Append("\\u000b");
          break;

        default:
          if (val < 0x20) {
            if (val < 0x10) {
              Append("\\u000");
              std::snprintf(buf, sizeof(buf), "%x", val);
              Append(buf);
            } else if (0x10 <= val && val < 0x20) {
              Append("\\u00");
              std::snprintf(buf, sizeof(buf), "%x", val);
              Append(buf);
            }
          } else if (0x80 <= val) {
            if (0x80 <= val && val < 0x1000) {
              Append("\\u0");
              std::snprintf(buf, sizeof(buf), "%x", val);
              Append(buf);
            } else if (0x1000 <= val) {
              Append("\\u");
              std::snprintf(buf, sizeof(buf), "%x", val);
              Append(buf);
            }
          } else {
            Append(val);
          }
          break;
      }
    }
  }
  UString out_;
};

} } }  // namespace iv::core::ast
#endif  // _IV_AST_SERIALIZER_H_
