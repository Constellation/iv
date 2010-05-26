#include <sstream>
#include <cstdio>
#include "alloc-inl.h"
#include "ast-visitor.h"
#include "ast.h"

namespace iv {
namespace core {

AstVisitor::AstVisitor() { }
AstVisitor::~AstVisitor() { }

void AstSerializer::Visit(Block* block) {
  out_.append("{\"type\":\"block\",\"body\":[");
  SpaceVector<Statement*>::type::const_iterator it = block->body().begin();
  const SpaceVector<Statement*>::type::const_iterator end = block->body().end();
  while (it != end) {
    (*it)->Accept(this);
    ++it;
    if (it != end) {
      out_.append(",");
    }
  }
  out_.append("]}");
}

void AstSerializer::Visit(FunctionStatement* func) {
  out_.append("{\"type\":\"function_statement\",\"def\":");
  func->function()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(VariableStatement* var) {
  out_.append("{\"type\":");
  if (var->IsConst()) {
    out_.append("\"const\",\"decls\":[");
  } else {
    out_.append("\"var\",\"decls\":[");
  }
  SpaceVector<Declaration*>::type::const_iterator it = var->decls().begin();
  const SpaceVector<Declaration*>::type::const_iterator
      end = var->decls().end();
  while (it != end) {
    (*it)->Accept(this);
    ++it;
    if (it != end) {
      out_.append(",");
    }
  }
  out_.append("]}");
}

void AstSerializer::Visit(Declaration* decl) {
  out_.append("{\"type\":\"decl\",\"name\":");
  decl->name()->Accept(this);
  out_.append(",\"exp\":");
  if (decl->expr()) {
    decl->expr()->Accept(this);
  }
  out_.append("}");
}

void AstSerializer::Visit(EmptyStatement* empty) {
  out_.append("{\"type\":\"empty\"}");
}

void AstSerializer::Visit(IfStatement* ifstmt) {
  out_.append("{\"type\":\"if\",\"cond\":");
  ifstmt->cond()->Accept(this);
  out_.append(",\"body\":");
  ifstmt->then_statement()->Accept(this);
  if (ifstmt->else_statement()) {
    out_.append(",\"else\":");
    ifstmt->else_statement()->Accept(this);
  }
  out_.append("}");
}

void AstSerializer::Visit(DoWhileStatement* dowhile) {
  out_.append("{\"type\":\"dowhile\",\"cond\":");
  dowhile->cond()->Accept(this);
  out_.append(",\"body\":");
  dowhile->body()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(WhileStatement* whilestmt) {
  out_.append("{\"type\":\"while\",\"cond\":");
  whilestmt->cond()->Accept(this);
  out_.append(",\"body\":");
  whilestmt->body()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(ForStatement* forstmt) {
  out_.append("{\"type\":\"for\"");
  if (forstmt->init() != NULL) {
    out_.append(",\"init\":");
    forstmt->init()->Accept(this);
  }
  if (forstmt->cond() != NULL) {
    out_.append(",\"cond\":");
    forstmt->cond()->Accept(this);
  }
  if (forstmt->next() != NULL) {
    out_.append(",\"next\":");
    forstmt->next()->Accept(this);
  }
  out_.append(",\"body\":");
  forstmt->body()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(ForInStatement* forstmt) {
  out_.append("{\"type\":\"forin\",\"each\":");
  forstmt->each()->Accept(this);
  out_.append(",\"enumerable\":");
  forstmt->enumerable()->Accept(this);
  out_.append(",\"body\":");
  forstmt->body()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(ContinueStatement* continuestmt) {
  out_.append("{\"type\":\"continue\"");
  if (continuestmt->label()) {
    out_.append(",\"label\":");
    continuestmt->label()->Accept(this);
  }
  out_.append("}");
}

void AstSerializer::Visit(BreakStatement* breakstmt) {
  out_.append("{\"type\":\"break\"");
  if (breakstmt->label()) {
    out_.append(",\"label\":");
    breakstmt->label()->Accept(this);
  }
  out_.append("}");
}

void AstSerializer::Visit(ReturnStatement* returnstmt) {
  out_.append("{\"type\":\"return\",\"exp\":");
  returnstmt->expr()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(WithStatement* withstmt) {
  out_.append("{\"type\":\"with\",\"context\":");
  withstmt->context()->Accept(this);
  out_.append(",\"body\":");
  withstmt->body()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(LabelledStatement* labelledstmt) {
  out_.append("{\"type\":\"labelled\",\"label\":");
  labelledstmt->label()->Accept(this);
  out_.append(",\"body\":");
  labelledstmt->body()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(CaseClause* clause) {
  if (clause->IsDefault()) {
    out_.append("{\"type\":\"default\"");
  } else {
    out_.append("{\"type\":\"case\",\"exp\":");
    clause->expr()->Accept(this);
  }
  if (clause->body()) {
    out_.append(",\"body\":");
    clause->body()->Accept(this);
  }
  out_.append("}");
}

void AstSerializer::Visit(SwitchStatement* switchstmt) {
  out_.append("{\"type\":\"switch\",\"exp\":");
  switchstmt->expr()->Accept(this);
  out_.append(",\"clauses\":[");
  SpaceVector<CaseClause*>::type::const_iterator
      it = switchstmt->clauses().begin();
  const SpaceVector<CaseClause*>::type::const_iterator
      end = switchstmt->clauses().end();
  while (it != end) {
    (*it)->Accept(this);
    ++it;
    if (it != end) {
      out_.append(",");
    }
  }
  out_.append("]}");
}

void AstSerializer::Visit(ThrowStatement* throwstmt) {
  out_.append("{\"type\":\"throw\",\"exp\":");
  throwstmt->expr()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(TryStatement* trystmt) {
  out_.append("{\"type\":\"try\",\"body\":");
  trystmt->body()->Accept(this);
  if (trystmt->catch_name()) {
    out_.append(",\"catch\":{\"type\":\"catch\",\"name\":");
    trystmt->catch_name()->Accept(this);
    out_.append(",\"body\":");
    trystmt->catch_block()->Accept(this);
    out_.append("}");
  }
  if (trystmt->finally_block()) {
    out_.append(",\"finally\":{\"type\":\"finally\",\"body\":");
    trystmt->finally_block()->Accept(this);
    out_.append("}");
  }
  out_.append("}");
}

void AstSerializer::Visit(DebuggerStatement* debuggerstmt) {
  out_.append("{\"type\":\"debugger\"}");
}

void AstSerializer::Visit(ExpressionStatement* exprstmt) {
  out_.append("{\"type\":\"expstatement\",\"exp\":");
  exprstmt->expr()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(Assignment* assign) {
  out_.append("{\"type\":\"assign\",\"op\":\"");
  out_.append(Token::Content(assign->op()));
  out_.append("\",\"left\":");
  assign->left()->Accept(this);
  out_.append(",\"right\":");
  assign->right()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(BinaryOperation* binary) {
  out_.append("{\"type\":\"binary\",\"op\":\"");
  out_.append(Token::Content(binary->op()));
  out_.append("\",\"left\":");
  binary->left()->Accept(this);
  out_.append(",\"right\":");
  binary->right()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(ConditionalExpression* cond) {
  out_.append("{\"type\":\"conditional\",\"cond\":");
  cond->cond()->Accept(this);
  out_.append(",\"left\":");
  cond->left()->Accept(this);
  out_.append(",\"right\":");
  cond->right()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(UnaryOperation* unary) {
  out_.append("{\"type\":\"unary\",\"op\":\"");
  out_.append(Token::Content(unary->op()));
  out_.append("\",\"exp\":");
  unary->expr()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(PostfixExpression* postfix) {
  out_.append("{\"type\":\"postfix\",\"op\":\"");
  out_.append(Token::Content(postfix->op()));
  out_.append("\",\"exp\":");
  postfix->expr()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(StringLiteral* literal) {
  out_.append("{\"type\":\"string\",\"value\":\"");
  DecodeString(literal->value());
  out_.append("\"}");
}

void AstSerializer::Visit(NumberLiteral* literal) {
  std::ostringstream sout;
  sout << literal->value();
  out_.append("{\"type\":\"number\",\"value\":\"");
  out_.append(sout.str().data());
  out_.append("\"}");
}

void AstSerializer::Visit(Identifier* literal) {
  out_.append("{\"type\":\"identifier\",\"value\":\"");
  out_.append(literal->value());
  out_.append("\"}");
}

void AstSerializer::Visit(ThisLiteral* literal) {
  out_.append("{\"type\":\"this\"}");
}

void AstSerializer::Visit(NullLiteral* literal) {
  out_.append("{\"type\":\"null\"}");
}

void AstSerializer::Visit(TrueLiteral* literal) {
  out_.append("{\"type\":\"true\"}");
}

void AstSerializer::Visit(FalseLiteral* literal) {
  out_.append("{\"type\":\"false\"}");
}

void AstSerializer::Visit(Undefined* literal) {
  out_.append("{\"type\":\"undefined\"}");
}

void AstSerializer::Visit(RegExpLiteral* literal) {
  out_.append("{\"type\":\"regexp\",\"value\":\"");
  DecodeString(literal->value());
  out_.append("\",\"flags\":\"");
  out_.append(literal->flags());
  out_.append("\"}");
}

void AstSerializer::Visit(ArrayLiteral* literal) {
  out_.append("{\"type\":\"array\",\"value\":[");
  SpaceVector<Expression*>::type::const_iterator it = literal->items().begin();
  const SpaceVector<Expression*>::type::const_iterator
      end = literal->items().end();
  while (it != end) {
    (*it)->Accept(this);
    ++it;
    if (it != end) {
      out_.append(",");
    }
  }
  out_.append("]}");
}

void AstSerializer::Visit(ObjectLiteral* literal) {
  out_.append("{\"type\":\"object\",\"value\":[");
  std::map<Identifier*, Expression*>::const_iterator
      it = literal->properties().begin();
  const std::map<Identifier*, Expression*>::const_iterator
      end = literal->properties().end();
  while (it != end) {
    out_.append("{\"type\":\"key_val\",\"key\":");
    it->first->Accept(this);
    out_.append(",\"val\":");
    it->second->Accept(this);
    out_.append("}");
    ++it;
    if (it == end) {
      break;
    }
    out_.append(",");
  }
  out_.append("]}");
}

void AstSerializer::Visit(FunctionLiteral* literal) {
  out_.append("{\"type\":\"function\",\"name\":\"");
  out_.append(literal->name());
  out_.append("\",\"params\":[");
  std::vector<Identifier*>::const_iterator
      it = literal->params().begin();
  const std::vector<Identifier*>::const_iterator
      end = literal->params().end();
  while (it != end) {
    (*it)->Accept(this);
    ++it;
    if (it == end) {
      break;
    }
    out_.append(",");
  }
  out_.append("],\"body\":[");
  std::vector<Statement*>::const_iterator
      it_body = literal->body().begin();
  const std::vector<Statement*>::const_iterator
      end_body = literal->body().end();
  while (it_body != end_body) {
    (*it_body)->Accept(this);
    ++it_body;
    if (it_body == end_body) {
      break;
    }
    out_.append(",");
  }
  out_.append("]}");
}

void AstSerializer::Visit(PropertyAccess* prop) {
  out_.append("{\"type\":\"property\",\"target\":");
  prop->target()->Accept(this);
  out_.append(",\"key\":");
  prop->key()->Accept(this);
  out_.append("}");
}

void AstSerializer::Visit(FunctionCall* call) {
  out_.append("{\"type\":\"funcall\",\"target\":");
  call->target()->Accept(this);
  out_.append(",\"args\":[");
  SpaceVector<Expression*>::type::const_iterator it = call->args().begin(),
                                           end = call->args().end();
  while (it != end) {
    (*it)->Accept(this);
    ++it;
    if (it == end) {
      break;
    }
    out_.append(",");
  }
  out_.append("]}");
}

void AstSerializer::Visit(ConstructorCall* call) {
  out_.append("{\"type\":\"new\",\"target\":");
  call->target()->Accept(this);
  out_.append(",\"args\":[");
  SpaceVector<Expression*>::type::const_iterator it = call->args().begin(),
                                           end = call->args().end();
  while (it != end) {
    (*it)->Accept(this);
    ++it;
    if (it == end) {
      break;
    }
    out_.append(",");
  }
  out_.append("]}");
}

void AstSerializer::DecodeString(const UnicodeString& ustr) {
  StringCharacterIterator it(ustr);
  int32_t val;
  char buf[5];
  it.setToStart();
  while (it.hasNext()) {
    val = it.next32PostInc();
    switch (val) {
      case '"':
        out_.append("\\\"");
        break;

      case '\\':
        out_.append("\\\\");
        break;

      case '/':
        out_.append("\\/");
        break;

      case '\b':
        out_.append("\\b");
        break;

      case '\f':
        out_.append("\\f");
        break;

      case '\n':
        out_.append("\\n");
        break;

      case '\r':
        out_.append("\\r");
        break;

      case '\t':
        out_.append("\\t");
        break;

      case '\x0B':  // \v
        out_.append("\\u000b");
        break;

      default:
        if (0x00 <= val && val < 0x20) {
          if (0x00 <= val && val < 0x10) {
            out_.append("\\u000");
            std::snprintf(buf, sizeof(buf), "%x", val);
            out_.append(buf);
          } else if (0x10 <= val && val < 0x20) {
            out_.append("\\u00");
            std::snprintf(buf, sizeof(buf), "%x", val);
            out_.append(buf);
          }
        } else if (0x80 <= val) {
          if (0x80 <= val && val < 0x1000) {
            out_.append("\\u0");
            std::snprintf(buf, sizeof(buf), "%x", val);
            out_.append(buf);
          } else if (0x1000 <= val) {
            out_.append("\\u");
            std::snprintf(buf, sizeof(buf), "%x", val);
            out_.append(buf);
          }
        } else {
          out_.append(val);
        }
        break;
    }
  }
}

} }  // namespace iv::core
