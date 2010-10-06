#include <sstream>
#include <cstdio>
#include <tr1/tuple>
#include "alloc-inl.h"
#include "ast-serializer.h"
#include "ast.h"
#include "ustream.h"

namespace iv {
namespace core {

void AstSerializer::Append(StringPiece str) {
  out_.append(str.begin(), str.end());
}

void AstSerializer::Append(UStringPiece str) {
  out_.append(str.begin(), str.end());
}

void AstSerializer::Append(UChar c) {
  out_.push_back(c);
}

void AstSerializer::Append(char c) {
  out_.push_back(c);
}

void AstSerializer::Visit(Block* block) {
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

void AstSerializer::Visit(FunctionStatement* func) {
  Append("{\"type\":\"function_statement\",\"def\":");
  func->function()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(VariableStatement* var) {
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

void AstSerializer::Visit(Declaration* decl) {
  Append("{\"type\":\"decl\",\"name\":");
  decl->name()->Accept(this);
  Append(",\"exp\":");
  if (decl->expr()) {
    decl->expr()->Accept(this);
  }
  Append('}');
}

void AstSerializer::Visit(EmptyStatement* empty) {
  Append("{\"type\":\"empty\"}");
}

void AstSerializer::Visit(IfStatement* ifstmt) {
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

void AstSerializer::Visit(DoWhileStatement* dowhile) {
  Append("{\"type\":\"dowhile\",\"cond\":");
  dowhile->cond()->Accept(this);
  Append(",\"body\":");
  dowhile->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(WhileStatement* whilestmt) {
  Append("{\"type\":\"while\",\"cond\":");
  whilestmt->cond()->Accept(this);
  Append(",\"body\":");
  whilestmt->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(ForStatement* forstmt) {
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

void AstSerializer::Visit(ForInStatement* forstmt) {
  Append("{\"type\":\"forin\",\"each\":");
  forstmt->each()->Accept(this);
  Append(",\"enumerable\":");
  forstmt->enumerable()->Accept(this);
  Append(",\"body\":");
  forstmt->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(ContinueStatement* continuestmt) {
  Append("{\"type\":\"continue\"");
  if (continuestmt->label()) {
    Append(",\"label\":");
    continuestmt->label()->Accept(this);
  }
  Append('}');
}

void AstSerializer::Visit(BreakStatement* breakstmt) {
  Append("{\"type\":\"break\"");
  if (breakstmt->label()) {
    Append(",\"label\":");
    breakstmt->label()->Accept(this);
  }
  Append('}');
}

void AstSerializer::Visit(ReturnStatement* returnstmt) {
  Append("{\"type\":\"return\",\"exp\":");
  returnstmt->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(WithStatement* withstmt) {
  Append("{\"type\":\"with\",\"context\":");
  withstmt->context()->Accept(this);
  Append(",\"body\":");
  withstmt->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(LabelledStatement* labelledstmt) {
  Append("{\"type\":\"labelled\",\"label\":");
  labelledstmt->label()->Accept(this);
  Append(",\"body\":");
  labelledstmt->body()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(CaseClause* clause) {
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

void AstSerializer::Visit(SwitchStatement* switchstmt) {
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

void AstSerializer::Visit(ThrowStatement* throwstmt) {
  Append("{\"type\":\"throw\",\"exp\":");
  throwstmt->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(TryStatement* trystmt) {
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

void AstSerializer::Visit(DebuggerStatement* debuggerstmt) {
  Append("{\"type\":\"debugger\"}");
}

void AstSerializer::Visit(ExpressionStatement* exprstmt) {
  Append("{\"type\":\"expstatement\",\"exp\":");
  exprstmt->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(Assignment* assign) {
  Append("{\"type\":\"assign\",\"op\":\"");
  Append(Token::Content(assign->op()));
  Append("\",\"left\":");
  assign->left()->Accept(this);
  Append(",\"right\":");
  assign->right()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(BinaryOperation* binary) {
  Append("{\"type\":\"binary\",\"op\":\"");
  Append(Token::Content(binary->op()));
  Append("\",\"left\":");
  binary->left()->Accept(this);
  Append(",\"right\":");
  binary->right()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(ConditionalExpression* cond) {
  Append("{\"type\":\"conditional\",\"cond\":");
  cond->cond()->Accept(this);
  Append(",\"left\":");
  cond->left()->Accept(this);
  Append(",\"right\":");
  cond->right()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(UnaryOperation* unary) {
  Append("{\"type\":\"unary\",\"op\":\"");
  Append(Token::Content(unary->op()));
  Append("\",\"exp\":");
  unary->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(PostfixExpression* postfix) {
  Append("{\"type\":\"postfix\",\"op\":\"");
  Append(Token::Content(postfix->op()));
  Append("\",\"exp\":");
  postfix->expr()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(StringLiteral* literal) {
  Append("{\"type\":\"string\",\"value\":\"");
  DecodeString(literal->value().begin(), literal->value().end());
  Append("\"}");
}

void AstSerializer::Visit(NumberLiteral* literal) {
  std::ostringstream sout;
  sout << literal->value();
  Append("{\"type\":\"number\",\"value\":\"");
  Append(sout.str());
  Append("\"}");
}

void AstSerializer::Visit(Identifier* literal) {
  Append("{\"type\":\"identifier\",\"value\":\"");
  Append(literal->value());
  Append("\"}");
}

void AstSerializer::Visit(ThisLiteral* literal) {
  Append("{\"type\":\"this\"}");
}

void AstSerializer::Visit(NullLiteral* literal) {
  Append("{\"type\":\"null\"}");
}

void AstSerializer::Visit(TrueLiteral* literal) {
  Append("{\"type\":\"true\"}");
}

void AstSerializer::Visit(FalseLiteral* literal) {
  Append("{\"type\":\"false\"}");
}

void AstSerializer::Visit(Undefined* literal) {
  Append("{\"type\":\"undefined\"}");
}

void AstSerializer::Visit(RegExpLiteral* literal) {
  Append("{\"type\":\"regexp\",\"value\":\"");
  DecodeString(literal->value().begin(), literal->value().end());
  Append("\",\"flags\":\"");
  Append(literal->flags());
  Append("\"}");
}

void AstSerializer::Visit(ArrayLiteral* literal) {
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

void AstSerializer::Visit(ObjectLiteral* literal) {
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

void AstSerializer::Visit(FunctionLiteral* literal) {
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

void AstSerializer::Visit(IndexAccess* prop) {
  Append("{\"type\":\"property\",\"target\":");
  prop->target()->Accept(this);
  Append(",\"key\":");
  prop->key()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(IdentifierAccess* prop) {
  Append("{\"type\":\"property\",\"target\":");
  prop->target()->Accept(this);
  Append(",\"key\":");
  prop->key()->Accept(this);
  Append('}');
}

void AstSerializer::Visit(FunctionCall* call) {
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

void AstSerializer::Visit(ConstructorCall* call) {
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
