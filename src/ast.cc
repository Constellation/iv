#include "ast.h"

namespace {

llvm::IRBuilder<> builder(llvm::getGlobalContext());

}

namespace iv {
namespace core {

AstNode::AstNode() { }

Block::Block(Space* factory) : body_(SpaceAllocator<Statement*>(factory)) { }

void Block::AddStatement(Statement* stmt) {
  body_.push_back(stmt);
}

void Block::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"block\",\"body\":[");
  SpaceVector<Statement*>::type::const_iterator it = body_.begin();
  const SpaceVector<Statement*>::type::const_iterator end = body_.end();
  while (it != end) {
    (*it)->Serialize(out);
    ++it;
    if (it != end) {
      out->append(",");
    }
  }
  out->append("]}");
}

FunctionStatement::FunctionStatement(FunctionLiteral* func)
  : function_(func) {
}

void FunctionStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"function_statement\",\"def\":");
  function_->Serialize(out);
  out->append("}");
}

VariableStatement::VariableStatement(Token::Type type, Space* factory)
  : is_const_(type == Token::CONST),
    decls_(SpaceAllocator<Declaration*>(factory)) { }

void VariableStatement::AddDeclaration(Declaration* decl) {
  decls_.push_back(decl);
}

void VariableStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":");
  if (is_const_) {
    out->append("\"const\",\"decls\":[");
  } else {
    out->append("\"var\",\"decls\":[");
  }
  SpaceVector<Declaration*>::type::const_iterator it = decls_.begin();
  const SpaceVector<Declaration*>::type::const_iterator end = decls_.end();
  while (it != end) {
    (*it)->Serialize(out);
    ++it;
    if (it != end) {
      out->append(",");
    }
  }
  out->append("]}");
}

Declaration::Declaration(Identifier* name, Expression* expr)
  : name_(name),
    expr_(expr) {
}

void Declaration::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"decl\",\"name\":");
  name_->Serialize(out);
  out->append(",\"exp\":");
  if (expr_) {
    expr_->Serialize(out);
  }
  out->append("}");
}

void EmptyStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"empty\"}");
}

IfStatement::IfStatement(Expression* cond, Statement* body)
  : cond_(cond),
    body_(body),
    else_(NULL) {
}

void IfStatement::SetElse(Statement* stmt) {
  else_ = stmt;
}

void IfStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"if\",\"cond\":");
  cond_->Serialize(out);
  out->append(",\"body\":");
  body_->Serialize(out);
  if (else_) {
    out->append(",\"else\":");
    else_->Serialize(out);
  }
  out->append("}");
}

llvm::Value* IfStatement::Codegen() {
  return NULL;
}

IterationStatement::IterationStatement(Statement* body)
  : body_(body) {
}

void IterationStatement::Serialize(UnicodeString* out) const {
  body_->Serialize(out);
}

DoWhileStatement::DoWhileStatement(Statement* body, Expression* cond)
  : IterationStatement(body),
    cond_(cond) {
}

void DoWhileStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"dowhile\",\"cond\":");
  cond_->Serialize(out);
  out->append(",\"body\":");
  IterationStatement::Serialize(out);
  out->append("}");
}

WhileStatement::WhileStatement(Statement* body, Expression* cond)
  : IterationStatement(body),
    cond_(cond) {
}

void WhileStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"while\",\"cond\":");
  cond_->Serialize(out);
  out->append(",\"body\":");
  IterationStatement::Serialize(out);
  out->append("}");
}

ForStatement::ForStatement(Statement* body)
  : IterationStatement(body),
    init_(NULL),
    cond_(NULL),
    next_(NULL) {
}

void ForStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"for\"");
  if (init_ != NULL) {
    out->append(",\"init\":");
    init_->Serialize(out);
  }
  if (cond_ != NULL) {
    out->append(",\"cond\":");
    cond_->Serialize(out);
  }
  if (next_ != NULL) {
    out->append(",\"next\":");
    next_->Serialize(out);
  }
  out->append(",\"body\":");
  IterationStatement::Serialize(out);
  out->append("}");
}

ForInStatement::ForInStatement(Statement* each,
                               Expression* enumerable, Statement* body)
  : IterationStatement(body),
    each_(each),
    enumerable_(enumerable) {
}

void ForInStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"forin\",\"each\":");
  each_->Serialize(out);
  out->append(",\"enumerable\":");
  enumerable_->Serialize(out);
  out->append(",\"body\":");
  IterationStatement::Serialize(out);
  out->append("}");
}

ContinueStatement::ContinueStatement()
  : label_(NULL) {
}

void ContinueStatement::SetLabel(Identifier* label) {
  label_ = label;
}

void ContinueStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"continue\"");
  if (label_) {
    out->append(",\"label\":");
    label_->Serialize(out);
  }
  out->append("}");
}

BreakStatement::BreakStatement()
  : label_(NULL) {
}

void BreakStatement::SetLabel(Identifier* label) {
  label_ = label;
}

void BreakStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"break\"");
  if (label_) {
    out->append(",\"label\":");
    label_->Serialize(out);
  }
  out->append("}");
}

ReturnStatement::ReturnStatement(Expression* expr)
  : expr_(expr) {
}

void ReturnStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"return\",\"exp\":");
  expr_->Serialize(out);
  out->append("}");
}

WithStatement::WithStatement(Expression* context, Statement* body)
  : context_(context),
    body_(body) {
}

void WithStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"with\",\"context\":");
  context_->Serialize(out);
  out->append(",\"body\":");
  body_->Serialize(out);
  out->append("}");
}

LabelledStatement::LabelledStatement(Expression* expr, Statement* body)
  : body_(body) {
  label_ = expr->AsLiteral()->AsIdentifier();
}

void LabelledStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"labelled\",\"label\":");
  label_->Serialize(out);
  out->append(",\"body\":");
  body_->Serialize(out);
  out->append("}");
}

SwitchStatement::SwitchStatement(Expression* expr, Space* factory)
  : expr_(expr),
    clauses_(SpaceAllocator<CaseClause*>(factory)) {
}

void SwitchStatement::AddCaseClause(CaseClause* clause) {
  clauses_.push_back(clause);
}

void SwitchStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"switch\",\"exp\":");
  expr_->Serialize(out);
  out->append(",\"clauses\":[");
  SpaceVector<CaseClause*>::type::const_iterator it = clauses_.begin();
  const SpaceVector<CaseClause*>::type::const_iterator end = clauses_.end();
  while (it != end) {
    (*it)->Serialize(out);
    ++it;
    if (it != end) {
      out->append(",");
    }
  }
  out->append("]}");
}

CaseClause::CaseClause()
  : expr_(NULL),
    body_(NULL),
    default_(false) {
}

void CaseClause::SetExpression(Expression* expr) {
  expr_ = expr;
}

void CaseClause::SetDefault() {
  default_ = true;
}

void CaseClause::SetStatement(Statement* stmt) {
  body_ = stmt;
}

void CaseClause::Serialize(UnicodeString* out) const {
  if (default_) {
    out->append("{\"type\":\"default\"");
  } else {
    out->append("{\"type\":\"case\",\"exp\":");
    expr_->Serialize(out);
  }
  if (body_) {
    out->append(",\"body\":");
    body_->Serialize(out);
  }
  out->append("}");
}

ThrowStatement::ThrowStatement(Expression* expr)
  : expr_(expr) {
}

void ThrowStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"throw\",\"exp\":");
  expr_->Serialize(out);
  out->append("}");
}

TryStatement::TryStatement(Block* block)
  : body_(block),
    catch_name_(NULL),
    catch_block_(NULL),
    finally_block_(NULL) {
}

void TryStatement::SetCatch(Identifier* name, Block* block) {
  catch_name_ = name;
  catch_block_ = block;
}

void TryStatement::SetFinally(Block* block) {
  finally_block_ = block;
}

void TryStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"try\",\"body\":");
  body_->Serialize(out);
  if (catch_name_) {
    out->append(",\"catch\":{\"type\":\"catch\",\"name\":");
    catch_name_->Serialize(out);
    out->append(",\"body\":");
    catch_block_->Serialize(out);
    out->append("}");
  }
  if (finally_block_) {
    out->append(",\"finally\":{\"type\":\"finally\",\"body\":");
    finally_block_->Serialize(out);
    out->append("}");
  }
  out->append("}");
}

void DebuggerStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"debugger\"}");
}

ExpressionStatement::ExpressionStatement(Expression* expr) : expr_(expr) { }

void ExpressionStatement::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"expstatement\",\"exp\":");
  expr_->Serialize(out);
  out->append("}");
}

Assignment::Assignment(Token::Type op,
                       Expression* left, Expression* right)
  : op_(op),
    left_(left),
    right_(right) {
}

void Assignment::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"assign\",\"op\":\"");
  out->append(Token::Content(op_));
  out->append("\",\"left\":");
  left_->Serialize(out);
  out->append(",\"right\":");
  right_->Serialize(out);
  out->append("}");
}



BinaryOperation::BinaryOperation(Token::Type op,
                                 Expression* left, Expression* right)
  : op_(op),
    left_(left),
    right_(right) {
}

void BinaryOperation::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"binary\",\"op\":\"");
  out->append(Token::Content(op_));
  out->append("\",\"left\":");
  left_->Serialize(out);
  out->append(",\"right\":");
  right_->Serialize(out);
  out->append("}");
}

ConditionalExpression::ConditionalExpression(Expression* cond,
                                             Expression* left,
                                             Expression* right)
  : cond_(cond), left_(left), right_(right) {
}

void ConditionalExpression::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"conditional\",\"cond\":");
  cond_->Serialize(out);
  out->append(",\"left\":");
  left_->Serialize(out);
  out->append(",\"right\":");
  right_->Serialize(out);
  out->append("}");
}

UnaryOperation::UnaryOperation(Token::Type op, Expression* expr)
  : op_(op),
    expr_(expr) {
}

void UnaryOperation::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"unary\",\"op\":\"");
  out->append(Token::Content(op_));
  out->append("\",\"exp\":");
  expr_->Serialize(out);
  out->append("}");
}

PostfixExpression::PostfixExpression(Token::Type op, Expression* expr)
  : op_(op),
    expr_(expr) {
}

void PostfixExpression::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"postfix\",\"op\":\"");
  out->append(Token::Content(op_));
  out->append("\",\"exp\":");
  expr_->Serialize(out);
  out->append("}");
}

StringLiteral::StringLiteral(const UChar* buffer)
  : value_(buffer) {
}

void StringLiteral::Serialize(UnicodeString* out) const {
  StringCharacterIterator it(value_);
  int32_t val;
  char buf[5];
  it.setToStart();
  out->append("{\"type\":\"string\",\"value\":\"");
  while (it.hasNext()) {
    val = it.next32PostInc();
    switch (val) {
      case '"':
        out->append("\\\"");
        break;
      case '\\':
        out->append("\\\\");
        break;
      case '/':
        out->append("\\/");
        break;
      case '\b':
        out->append("\\b");
        break;
      case '\f':
        out->append("\\f");
        break;
      case '\n':
        out->append("\\n");
        break;
      case '\r':
        out->append("\\r");
        break;
      case '\t':
        out->append("\\t");
        break;
      case '\x0B':  // \v
        out->append("\\u000b");
        break;
      default:
        if (0x00 <= val && val < 0x20) {
          if (0x00 <= val && val < 0x10) {
            out->append("\\u000");
            snprintf(buf, sizeof(buf), "%x", val);
            out->append(buf);
          } else if (0x10 <= val && val < 0x20) {
            out->append("\\u00");
            snprintf(buf, sizeof(buf), "%x", val);
            out->append(buf);
          }
        } else if (0x80 <= val) {
          if (0x80 <= val && val < 0x1000) {
            out->append("\\u0");
            snprintf(buf, sizeof(buf), "%x", val);
            out->append(buf);
          } else if (0x1000 <= val) {
            out->append("\\u");
            snprintf(buf, sizeof(buf), "%x", val);
            out->append(buf);
          }
        } else {
          out->append(val);
        }
        break;
    }
  }
  out->append("\"}");
}

NumberLiteral::NumberLiteral(const double & val)
  : value_(val) {
}

llvm::Value* NumberLiteral::Codegen() {
  return llvm::ConstantFP::get(
      llvm::Type::getDoubleTy(llvm::getGlobalContext()), value_);
}

void NumberLiteral::Serialize(UnicodeString* out) const {
  std::ostringstream sout;
  sout << value_;
  out->append("{\"type\":\"number\",\"value\":\"");
  out->append(sout.str().data());
  out->append("\"}");
}

Identifier::Identifier(const UChar* buffer)
  : value_(buffer) {
}

Identifier::Identifier(const char* buffer)
  : value_(buffer) {
}

void Identifier::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"identifier\",\"value\":\"");
  out->append(value_);
  out->append("\"}");
}

void ThisLiteral::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"this\"}");
}

void NullLiteral::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"null\"}");
}

void TrueLiteral::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"true\"}");
}

void FalseLiteral::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"false\"}");
}

void Undefined::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"undefined\"}");
}

RegExpLiteral::RegExpLiteral(const UChar* buffer)
  : value_(buffer),
    flags_() {
}

void RegExpLiteral::SetFlags(const UChar* flags) {
  flags_ = flags;
}

void RegExpLiteral::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"regexp\",\"value\":\"");
  StringCharacterIterator it(value_);
  int32_t val;
  char buf[5];
  it.setToStart();
  while (it.hasNext()) {
    val = it.next32PostInc();
    switch (val) {
      case '"':
        out->append("\\\"");
        break;
      case '\\':
        out->append("\\\\");
        break;
      case '/':
        out->append("\\/");
        break;
      case '\b':
        out->append("\\b");
        break;
      case '\f':
        out->append("\\f");
        break;
      case '\n':
        out->append("\\n");
        break;
      case '\r':
        out->append("\\r");
        break;
      case '\t':
        out->append("\\t");
        break;
      case '\x0B':  // \v
        out->append("\\u000b");
        break;
      default:
        if (0x00 <= val && val < 0x20) {
          if (0x00 <= val && val < 0x10) {
            out->append("\\u000");
            snprintf(buf, sizeof(buf), "%x", val);
            out->append(buf);
          } else if (0x10 <= val && val < 0x20) {
            out->append("\\u00");
            snprintf(buf, sizeof(buf), "%x", val);
            out->append(buf);
          }
        } else if (0x80 <= val) {
          if (0x80 <= val && val < 0x1000) {
            out->append("\\u0");
            snprintf(buf, sizeof(buf), "%x", val);
            out->append(buf);
          } else if (0x1000 <= val) {
            out->append("\\u");
            snprintf(buf, sizeof(buf), "%x", val);
            out->append(buf);
          }
        } else {
          out->append(val);
        }
        break;
    }
  }
  out->append("\",\"flags\":\"");
  out->append(flags_);
  out->append("\"}");
}

ArrayLiteral::ArrayLiteral(Space* factory)
  : items_(SpaceAllocator<Expression*>(factory)) {
}

void ArrayLiteral::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"array\",\"value\":[");
  SpaceVector<Expression*>::type::const_iterator it = items_.begin();
  const SpaceVector<Expression*>::type::const_iterator end = items_.end();
  while (it != end) {
    (*it)->Serialize(out);
    ++it;
    if (it != end) {
      out->append(",");
    }
  }
  out->append("]}");
}

ObjectLiteral::ObjectLiteral() : properties_() {
}

void ObjectLiteral::AddProperty(Identifier* key, Expression* val) {
  properties_.insert(std::map<Identifier*, Expression*>::value_type(key, val));
}

void ObjectLiteral::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"object\",\"value\":[");
  std::map<Identifier*, Expression*>::const_iterator it = properties_.begin();
  const std::map<Identifier*, Expression*>
      ::const_iterator end = properties_.end();
  while (it != end) {
    out->append("{\"type\":\"key_val\",\"key\":");
    it->first->Serialize(out);
    out->append(",\"val\":");
    it->second->Serialize(out);
    out->append("}");
    ++it;
    if (it == end) {
      break;
    }
    out->append(",");
  }
  out->append("]}");
}

FunctionLiteral::FunctionLiteral(Type type)
  : name_(""),
    type_(type),
    params_(),
    body_() {
}

void FunctionLiteral::AddParameter(Identifier* param) {
  params_.push_back(param);
}

void FunctionLiteral::AddStatement(Statement* stmt) {
  body_.push_back(stmt);
}

void FunctionLiteral::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"function\",\"name\":\"");
  out->append(name_);
  out->append("\",\"params\":[");
  std::vector<Identifier*>::const_iterator it = params_.begin();
  const std::vector<Identifier*>::const_iterator end = params_.end();
  while (it != end) {
    (*it)->Serialize(out);
    ++it;
    if (it == end) {
      break;
    }
    out->append(",");
  }
  out->append("],\"body\":[");
  std::vector<Statement*>::const_iterator it_body = body_.begin();
  const std::vector<Statement*>::const_iterator end_body = body_.end();
  while (it_body != end_body) {
    (*it_body)->Serialize(out);
    ++it_body;
    if (it_body == end_body) {
      break;
    }
    out->append(",");
  }
  out->append("]}");
}

Property::Property(Expression* obj, Expression* key)
  : obj_(obj),
    key_(key) {
}

void Property::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"property\",\"target\":");
  obj_->Serialize(out);
  out->append(",\"key\":");
  key_->Serialize(out);
  out->append("}");
}

Call::Call(Expression* target, Space* factory)
  : target_(target),
    args_(SpaceAllocator<Expression*>(factory)) {
}

ConstructorCall::ConstructorCall(Expression* target, Space* factory)
  : Call(target, factory) {
}

void ConstructorCall::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"new\",\"target\":");
  target_->Serialize(out);
  out->append(",\"args\":[");
  SpaceVector<Expression*>::type::const_iterator it = args_.begin(),
                                           end = args_.end();
  while (it != end) {
    (*it)->Serialize(out);
    ++it;
    if (it == end) {
      break;
    }
    out->append(",");
  }
  out->append("]}");
}

FunctionCall::FunctionCall(Expression* target, Space* factory)
  : Call(target, factory) {
}

void FunctionCall::Serialize(UnicodeString* out) const {
  out->append("{\"type\":\"funcall\",\"target\":");
  target_->Serialize(out);
  out->append(",\"args\":[");
  SpaceVector<Expression*>::type::const_iterator it = args_.begin(),
                                           end = args_.end();
  while (it != end) {
    (*it)->Serialize(out);
    ++it;
    if (it == end) {
      break;
    }
    out->append(",");
  }
  out->append("]}");
}


} }  // namespace iv::core

