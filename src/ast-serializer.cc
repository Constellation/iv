#include <sstream>
#include <cstdio>
#include <tr1/tuple>
#include "alloc-inl.h"
#include "ast-serializer.h"
#include "ast.h"
#include "ustream.h"

namespace iv {
namespace core {

void AstSerializer::Append(const StringPiece& str) {
  out_.append(str.begin(), str.end());
}

void AstSerializer::Append(const UStringPiece& str) {
  out_.append(str.begin(), str.end());
}

void AstSerializer::Append(UChar c) {
  out_.push_back(c);
}

void AstSerializer::Append(char c) {
  out_.push_back(c);
}

void AstSerializer::Visit(const Block* block) {
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

void AstSerializer::Visit(const FunctionStatement* func) {
  Append("{\"type\":\"function_statement\",\"def\":");
  func->function()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const VariableStatement* var) {
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

void AstSerializer::Visit(const Declaration* decl) {
  Append("{\"type\":\"decl\",\"name\":");
  decl->name()->Accept(this);
  Append(",\"exp\":");
  if (decl->expr()) {
    decl->expr()->Accept(this);
  }
  Append('}');
}

void AstSerializer::Visit(const EmptyStatement* empty) {
  Append("{\"type\":\"empty\"}");
}

void AstSerializer::Visit(const IfStatement* ifstmt) {
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

void AstSerializer::Visit(const DoWhileStatement* dowhile) {
  Append("{\"type\":\"dowhile\",\"cond\":");
  dowhile->cond()->Accept(this);
  Append(",\"body\":");
  dowhile->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const WhileStatement* whilestmt) {
  Append("{\"type\":\"while\",\"cond\":");
  whilestmt->cond()->Accept(this);
  Append(",\"body\":");
  whilestmt->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const ForStatement* forstmt) {
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

void AstSerializer::Visit(const ForInStatement* forstmt) {
  Append("{\"type\":\"forin\",\"each\":");
  forstmt->each()->Accept(this);
  Append(",\"enumerable\":");
  forstmt->enumerable()->Accept(this);
  Append(",\"body\":");
  forstmt->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const ContinueStatement* continuestmt) {
  Append("{\"type\":\"continue\"");
  if (continuestmt->label()) {
    Append(",\"label\":");
    continuestmt->label()->Accept(this);
  }
  Append('}');
}

void AstSerializer::Visit(const BreakStatement* breakstmt) {
  Append("{\"type\":\"break\"");
  if (breakstmt->label()) {
    Append(",\"label\":");
    breakstmt->label()->Accept(this);
  }
  Append('}');
}

void AstSerializer::Visit(const ReturnStatement* returnstmt) {
  Append("{\"type\":\"return\",\"exp\":");
  returnstmt->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const WithStatement* withstmt) {
  Append("{\"type\":\"with\",\"context\":");
  withstmt->context()->Accept(this);
  Append(",\"body\":");
  withstmt->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const LabelledStatement* labelledstmt) {
  Append("{\"type\":\"labelled\",\"label\":");
  labelledstmt->label()->Accept(this);
  Append(",\"body\":");
  labelledstmt->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const CaseClause* clause) {
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

void AstSerializer::Visit(const SwitchStatement* switchstmt) {
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

void AstSerializer::Visit(const ThrowStatement* throwstmt) {
  Append("{\"type\":\"throw\",\"exp\":");
  throwstmt->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const TryStatement* trystmt) {
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

void AstSerializer::Visit(const DebuggerStatement* debuggerstmt) {
  Append("{\"type\":\"debugger\"}");
}

void AstSerializer::Visit(const ExpressionStatement* exprstmt) {
  Append("{\"type\":\"expstatement\",\"exp\":");
  exprstmt->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const Assignment* assign) {
  Append("{\"type\":\"assign\",\"op\":\"");
  Append(Token::ToString(assign->op()));
  Append("\",\"left\":");
  assign->left()->Accept(this);
  Append(",\"right\":");
  assign->right()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const BinaryOperation* binary) {
  Append("{\"type\":\"binary\",\"op\":\"");
  Append(Token::ToString(binary->op()));
  Append("\",\"left\":");
  binary->left()->Accept(this);
  Append(",\"right\":");
  binary->right()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const ConditionalExpression* cond) {
  Append("{\"type\":\"conditional\",\"cond\":");
  cond->cond()->Accept(this);
  Append(",\"left\":");
  cond->left()->Accept(this);
  Append(",\"right\":");
  cond->right()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const UnaryOperation* unary) {
  Append("{\"type\":\"unary\",\"op\":\"");
  Append(Token::ToString(unary->op()));
  Append("\",\"exp\":");
  unary->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const PostfixExpression* postfix) {
  Append("{\"type\":\"postfix\",\"op\":\"");
  Append(Token::ToString(postfix->op()));
  Append("\",\"exp\":");
  postfix->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const StringLiteral* literal) {
  Append("{\"type\":\"string\",\"value\":\"");
  DecodeString(literal->value().begin(), literal->value().end());
  Append("\"}");
}

void AstSerializer::Visit(const NumberLiteral* literal) {
  std::ostringstream sout;
  sout << literal->value();
  Append("{\"type\":\"number\",\"value\":\"");
  Append(sout.str());
  Append("\"}");
}

void AstSerializer::Visit(const Identifier* literal) {
  Append("{\"type\":\"identifier\",\"value\":\"");
  Append(literal->value());
  Append("\"}");
}

void AstSerializer::Visit(const ThisLiteral* literal) {
  Append("{\"type\":\"this\"}");
}

void AstSerializer::Visit(const NullLiteral* literal) {
  Append("{\"type\":\"null\"}");
}

void AstSerializer::Visit(const TrueLiteral* literal) {
  Append("{\"type\":\"true\"}");
}

void AstSerializer::Visit(const FalseLiteral* literal) {
  Append("{\"type\":\"false\"}");
}

void AstSerializer::Visit(const Undefined* literal) {
  Append("{\"type\":\"undefined\"}");
}

void AstSerializer::Visit(const RegExpLiteral* literal) {
  Append("{\"type\":\"regexp\",\"value\":\"");
  DecodeString(literal->value().begin(), literal->value().end());
  Append("\",\"flags\":\"");
  Append(literal->flags());
  Append("\"}");
}

void AstSerializer::Visit(const ArrayLiteral* literal) {
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

void AstSerializer::Visit(const ObjectLiteral* literal) {
  using std::tr1::get;
  Append("{\"type\":\"object\",\"value\":[");
  ObjectLiteral::Properties::const_iterator it = literal->properties().begin();
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

void AstSerializer::Visit(const FunctionLiteral* literal) {
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

void AstSerializer::Visit(const IndexAccess* prop) {
  Append("{\"type\":\"property\",\"target\":");
  prop->target()->Accept(this);
  Append(",\"key\":");
  prop->key()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const IdentifierAccess* prop) {
  Append("{\"type\":\"property\",\"target\":");
  prop->target()->Accept(this);
  Append(",\"key\":");
  prop->key()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(const FunctionCall* call) {
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

void AstSerializer::Visit(const ConstructorCall* call) {
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

} }  // namespace iv::core
