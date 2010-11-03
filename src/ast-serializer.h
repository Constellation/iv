#ifndef _IV_AST_SERIALIZER_H_
#define _IV_AST_SERIALIZER_H_
#include <cstdio>
#include <ostream>  // NOLINT
#include <sstream>
#include <tr1/tuple>
#include "uchar.h"
#include "alloc-inl.h"
#include "ast.h"
#include "ast-visitor.h"
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
namespace iv {
namespace core {

class AstSerializer: public ConstAstVisitor {
 public:

  typedef void ReturnType;

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
    AstNode::Statements::const_iterator it = block->body().begin();
    const AstNode::Statements::const_iterator end = block->body().end();
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

  void Visit(const VariableStatement* var) {
    Append("{\"type\":");
    if (var->IsConst()) {
      Append("\"const\",\"decls\":[");
    } else {
      Append("\"var\",\"decls\":[");
    }
    AstNode::Declarations::const_iterator it = var->decls().begin();
    const AstNode::Declarations::const_iterator end = var->decls().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it != end) {
        Append(',');
      }
    }
    Append("]}");
  }

  void Visit(const Declaration* decl) {
    Append("{\"type\":\"decl\",\"name\":");
    decl->name()->Accept(this);
    Append(",\"exp\":");
    if (decl->expr()) {
      decl->expr()->Accept(this);
    }
    Append('}');
  }

  void Visit(const EmptyStatement* empty) {
    Append("{\"type\":\"empty\"}");
  }

  void Visit(const IfStatement* ifstmt) {
    Append("{\"type\":\"if\",\"cond\":");
    ifstmt->cond()->Accept(this);
    Append(",\"body\":");
    ifstmt->then_statement()->Accept(this);
    if (ifstmt->else_statement()) {
      Append(",\"else\":");
      ifstmt->else_statement()->Accept(this);
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
    if (forstmt->init() != NULL) {
      Append(",\"init\":");
      forstmt->init()->Accept(this);
    }
    if (forstmt->cond() != NULL) {
      Append(",\"cond\":");
      forstmt->cond()->Accept(this);
    }
    if (forstmt->next() != NULL) {
      Append(",\"next\":");
      forstmt->next()->Accept(this);
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
    if (continuestmt->label()) {
      Append(",\"label\":");
      continuestmt->label()->Accept(this);
    }
    Append('}');
  }

  void Visit(const BreakStatement* breakstmt) {
    Append("{\"type\":\"break\"");
    if (breakstmt->label()) {
      Append(",\"label\":");
      breakstmt->label()->Accept(this);
    }
    Append('}');
  }

  void Visit(const ReturnStatement* returnstmt) {
    Append("{\"type\":\"return\",\"exp\":");
    returnstmt->expr()->Accept(this);
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

  void Visit(const CaseClause* clause) {
    if (clause->IsDefault()) {
      Append("{\"type\":\"default\"");
    } else {
      Append("{\"type\":\"case\",\"exp\":");
      clause->expr()->Accept(this);
    }
    Append(",\"body\"[");
    AstNode::Statements::const_iterator it = clause->body().begin();
    const AstNode::Statements::const_iterator end = clause->body().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it != end) {
        Append(',');
      }
    }
    Append("]}");
  }

  void Visit(const SwitchStatement* switchstmt) {
    Append("{\"type\":\"switch\",\"exp\":");
    switchstmt->expr()->Accept(this);
    Append(",\"clauses\":[");
    SwitchStatement::CaseClauses::const_iterator
        it = switchstmt->clauses().begin();
    const SwitchStatement::CaseClauses::const_iterator
        end = switchstmt->clauses().end();
    while (it != end) {
      (*it)->Accept(this);
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
    if (trystmt->catch_name()) {
      Append(",\"catch\":{\"type\":\"catch\",\"name\":");
      trystmt->catch_name()->Accept(this);
      Append(",\"body\":");
      trystmt->catch_block()->Accept(this);
      Append('}');
    }
    if (trystmt->finally_block()) {
      Append(",\"finally\":{\"type\":\"finally\",\"body\":");
      trystmt->finally_block()->Accept(this);
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

  void Visit(const Undefined* literal) {
    Append("{\"type\":\"undefined\"}");
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
    AstNode::Expressions::const_iterator it = literal->items().begin();
    const AstNode::Expressions::const_iterator end = literal->items().end();
    bool previous_is_elision = false;
    while (it != end) {
      if ((*it)) {
        (*it)->Accept(this);
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
    ObjectLiteral::Properties::const_iterator
        it = literal->properties().begin();
    const ObjectLiteral::Properties::const_iterator
      end = literal->properties().end();
    while (it != end) {
      Append("{\"type\":");
      const ObjectLiteral::PropertyDescriptorType type(get<0>(*it));
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
    if (literal->name()) {
      literal->name()->Accept(this);
    }
    Append(",\"params\":[");
    AstNode::Identifiers::const_iterator it = literal->params().begin();
    const AstNode::Identifiers::const_iterator end = literal->params().end();
    while (it != end) {
      (*it)->Accept(this);
      ++it;
      if (it == end) {
        break;
      }
      Append(',');
    }
    Append("],\"body\":[");
    AstNode::Statements::const_iterator it_body = literal->body().begin();
    const AstNode::Statements::const_iterator end_body = literal->body().end();
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
    AstNode::Expressions::const_iterator it = call->args().begin();
    const AstNode::Expressions::const_iterator end = call->args().end();
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
    AstNode::Expressions::const_iterator it = call->args().begin();
    const AstNode::Expressions::const_iterator end = call->args().end();
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

} }  // namespace iv::core
#endif  // _IV_AST_SERIALIZER_H_
