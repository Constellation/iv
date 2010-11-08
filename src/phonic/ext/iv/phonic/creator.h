#ifndef _IV_PHONIC_CREATOR_H_
#define _IV_PHONIC_CREATOR_H_
#include <ruby.h>
#include <iv/ast-visitor.h>
#include <iv/utils.h>
#include "factory.h"
#include "encoding.h"
#define SYM(str) ID2SYM(rb_intern(str))
// create RubyObject of AST from iv AST
namespace iv {
namespace phonic {

class Creator : public iv::core::ast::AstVisitor<AstFactory>::const_type {
 public:
  Creator() { }

  VALUE Result(const FunctionLiteral* global) {
    VALUE array = rb_ary_new();
    for (Statements::const_iterator it = global->body().begin(),
         last = global->body().end(); it != last; ++it) {
      (*it)->Accept(this);
      rb_ary_push(array, ret_);
    }
    return array;
  }

  void Visit(const Block* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("Block"));
    VALUE array = rb_ary_new();
    for (Statements::const_iterator it = stmt->body().begin(),
         last = stmt->body().end(); it != last; ++it) {
      (*it)->Accept(this);
      rb_ary_push(array, ret_);
    }
    rb_hash_aset(hash, SYM("body"), array);
    ret_ = hash;
  }

  void Visit(const FunctionStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("FunctionStatement"));
    Visit(stmt->function());
    rb_hash_aset(hash, SYM("body"), ret_);
    ret_ = hash;
  }

  void Visit(const VariableStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("VariableStatement"));
    rb_hash_aset(hash, SYM("const"), stmt->IsConst() ? Qtrue : Qfalse);
    VALUE array = rb_ary_new();
    for (Declarations::const_iterator it = stmt->decls().begin(),
         last = stmt->decls().end(); it != last; ++it) {
      VALUE decl = rb_hash_new();
      rb_hash_aset(decl, SYM("type"), rb_str_new_cstr("Declaration"));
      Visit((*it)->name());
      rb_hash_aset(decl, SYM("name"), ret_);
      if ((*it)->expr()) {
        (*it)->expr()->Accept(this);
        rb_hash_aset(decl, SYM("expr"), ret_);
      }
      rb_ary_push(array, decl);
    }
    rb_hash_aset(hash, SYM("body"), array);
    ret_ = hash;
  }

  void Visit(const EmptyStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("EmptyStatement"));
    ret_ = hash;
  }

  void Visit(const IfStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("IfStatement"));
    stmt->cond()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    stmt->then_statement()->Accept(this);
    rb_hash_aset(hash, SYM("then"), ret_);
    if (stmt->else_statement()) {
      stmt->else_statement()->Accept(this);
      rb_hash_aset(hash, SYM("else"), ret_);
    }
    ret_ = hash;
  }

  void Visit(const DoWhileStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("DoWhileStatement"));
    stmt->cond()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    ret_ = hash;
  }

  void Visit(const WhileStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("WhileStatement"));
    stmt->cond()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    ret_ = hash;
  }

  void Visit(const ForStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ForStatement"));
    if (stmt->init()) {
      stmt->init()->Accept(this);
      rb_hash_aset(hash, SYM("init"), ret_);
    }
    if (stmt->cond()) {
      stmt->cond()->Accept(this);
      rb_hash_aset(hash, SYM("cond"), ret_);
    }
    if (stmt->next()) {
      stmt->next()->Accept(this);
      rb_hash_aset(hash, SYM("next"), ret_);
    }
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    ret_ = hash;
  }

  void Visit(const ForInStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ForInStatement"));
    stmt->each()->Accept(this);
    rb_hash_aset(hash, SYM("each"), ret_);
    stmt->enumerable()->Accept(this);
    rb_hash_aset(hash, SYM("enum"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    ret_ = hash;
  }

  void Visit(const ContinueStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ContinueStatement"));
    if (stmt->label()) {
      stmt->label()->Accept(this);
      rb_hash_aset(hash, SYM("label"), ret_);
    }
    ret_ = hash;
  }

  void Visit(const BreakStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("BreakStatement"));
    if (stmt->label()) {
      stmt->label()->Accept(this);
      rb_hash_aset(hash, SYM("label"), ret_);
    }
    ret_ = hash;
  }

  void Visit(const ReturnStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ReturnStatement"));
    stmt->expr()->Accept(this);
    rb_hash_aset(hash, SYM("expr"), ret_);
    ret_ = hash;
  }

  void Visit(const WithStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("WithStatement"));
    stmt->context()->Accept(this);
    rb_hash_aset(hash, SYM("context"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    ret_ = hash;
  }

  void Visit(const LabelledStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("LabelledStatement"));
    stmt->label()->Accept(this);
    rb_hash_aset(hash, SYM("label"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    ret_ = hash;
  }

  void Visit(const SwitchStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("SwitchStatement"));
    stmt->expr()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    VALUE array = rb_ary_new();
    for (CaseClauses::const_iterator it = stmt->clauses().begin(),
         last = stmt->clauses().end(); it != last; ++it) {
      VALUE clause = rb_hash_new();
      if ((*it)->IsDefault()) {
        rb_hash_aset(clause, SYM("type"), rb_str_new_cstr("Default"));
      } else {
        rb_hash_aset(clause, SYM("type"), rb_str_new_cstr("Case"));
        (*it)->expr()->Accept(this);
        rb_hash_aset(clause, SYM("expr"), ret_);
      }
      VALUE stmts = rb_ary_new();
      for (Statements::const_iterator st = (*it)->body().begin(),
           stlast = (*it)->body().end(); st != stlast; ++st) {
        (*st)->Accept(this);
        rb_ary_push(stmts, ret_);
      }
      rb_hash_aset(clause, SYM("body"), stmts);
      rb_ary_push(array, ret_);
    }
    rb_hash_aset(hash, SYM("clauses"), array);
    ret_ = hash;
  }

  void Visit(const ThrowStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ThrowStatement"));
    ret_ = hash;
  }

  void Visit(const TryStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("TryStatement"));
    ret_ = hash;
  }

  void Visit(const DebuggerStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("DebuggerStatement"));
    ret_ = hash;
  }

  void Visit(const ExpressionStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ExpressionStatement"));
    stmt->expr()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    ret_ = hash;
  }

  void Visit(const Assignment* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("Assignment"));
    rb_hash_aset(hash, SYM("op"),
                 rb_str_new_cstr(core::Token::ToString(expr->op())));
    expr->left()->Accept(this);
    rb_hash_aset(hash, SYM("left"), ret_);
    expr->right()->Accept(this);
    rb_hash_aset(hash, SYM("right"), ret_);
    ret_ = hash;
  }

  void Visit(const BinaryOperation* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("BinaryOperation"));
    rb_hash_aset(hash, SYM("op"),
                 rb_str_new_cstr(core::Token::ToString(expr->op())));
    expr->left()->Accept(this);
    rb_hash_aset(hash, SYM("left"), ret_);
    expr->right()->Accept(this);
    rb_hash_aset(hash, SYM("right"), ret_);
    ret_ = hash;
  }

  void Visit(const ConditionalExpression* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ConditionalExpression"));
    expr->cond()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    expr->left()->Accept(this);
    rb_hash_aset(hash, SYM("left"), ret_);
    expr->right()->Accept(this);
    rb_hash_aset(hash, SYM("right"), ret_);
    ret_ = hash;
  }

  void Visit(const UnaryOperation* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("UnaryOperation"));
    rb_hash_aset(hash, SYM("op"),
                 rb_str_new_cstr(core::Token::ToString(expr->op())));
    expr->expr()->Accept(this);
    rb_hash_aset(hash, SYM("expr"), ret_);
    ret_ = hash;
  }

  void Visit(const PostfixExpression* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("PostfixExpression"));
    rb_hash_aset(hash, SYM("op"),
                 rb_str_new_cstr(core::Token::ToString(expr->op())));
    expr->expr()->Accept(this);
    rb_hash_aset(hash, SYM("expr"), ret_);
    ret_ = hash;
  }

  void Visit(const StringLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("StringLiteral"));
    const StringLiteral::value_type& str = literal->value();
    rb_hash_aset(hash, SYM("value"),
                 Encoding::ConvertToDefaultInternal(
                     rb_enc_str_new(
                         reinterpret_cast<const char*>(str.data()),
                         str.size()*2,
                         rb_to_encoding(Encoding::UTF16Encoding()))));
    ret_ = hash;
  }

  void Visit(const NumberLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("NumberLiteral"));
    rb_hash_aset(hash, SYM("value"), rb_float_new(literal->value()));
    ret_ = hash;
  }

  void Visit(const Identifier* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("Identifier"));
    const Identifier::value_type& str = literal->value();
    rb_hash_aset(hash, SYM("value"),
                 Encoding::ConvertToDefaultInternal(
                     rb_enc_str_new(
                         reinterpret_cast<const char*>(str.data()),
                         str.size()*2,
                         rb_to_encoding(Encoding::UTF16Encoding()))));
    ret_ = hash;
  }

  void Visit(const ThisLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ThisLiteral"));
    ret_ = hash;
  }

  void Visit(const NullLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("NullLiteral"));
    ret_ = hash;
  }

  void Visit(const TrueLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("TrueLiteral"));
    ret_ = hash;
  }

  void Visit(const FalseLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("FalseLiteral"));
    ret_ = hash;
  }

  void Visit(const Undefined* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("Undefined"));
    ret_ = hash;
  }

  void Visit(const RegExpLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("RegExpLiteral"));
    const RegExpLiteral::value_type& content = literal->value();
    rb_hash_aset(hash, SYM("value"),
                 Encoding::ConvertToDefaultInternal(
                     rb_enc_str_new(
                         reinterpret_cast<const char*>(content.data()),
                         content.size()*2,
                         rb_to_encoding(Encoding::UTF16Encoding()))));
    const RegExpLiteral::value_type& flags = literal->flags();
    rb_hash_aset(hash, SYM("flags"),
                 Encoding::ConvertToDefaultInternal(
                     rb_enc_str_new(
                         reinterpret_cast<const char*>(flags.data()),
                         flags.size()*2,
                         rb_to_encoding(Encoding::UTF16Encoding()))));
    ret_ = hash;
  }

  void Visit(const ArrayLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ArrayLiteral"));
    VALUE array = rb_ary_new();
    for (Expressions::const_iterator it = literal->items().begin(),
         last = literal->items().end(); it != last; ++it) {
      (*it)->Accept(this);
      rb_ary_push(array, ret_);
    }
    rb_hash_aset(hash, SYM("value"), array);
    ret_ = hash;
  }

  void Visit(const ObjectLiteral* literal) {
    using std::tr1::get;
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ObjectLiteral"));
    VALUE array = rb_ary_new();
    for (ObjectLiteral::Properties::const_iterator it = literal->properties().begin(),
         last = literal->properties().end(); it != last; ++it) {
      VALUE item = rb_hash_new();
      switch (get<0>(*it)) {
        case ObjectLiteral::DATA:
          rb_hash_aset(item,
                       SYM("type"),
                       rb_str_new_cstr("DataProperty"));
          break;

        case ObjectLiteral::SET:
          rb_hash_aset(item,
                       SYM("type"),
                       rb_str_new_cstr("SetterProperty"));
          break;

        case ObjectLiteral::GET:
          rb_hash_aset(item,
                       SYM("type"),
                       rb_str_new_cstr("GetterProperty"));
          break;
      }
      get<1>(*it)->Accept(this);
      rb_hash_aset(item,
                   SYM("key"),
                   ret_);
      get<2>(*it)->Accept(this);
      rb_hash_aset(item,
                   SYM("value"),
                   ret_);
      rb_ary_push(array, item);
    }
    rb_hash_aset(hash, SYM("value"), array);
    ret_ = hash;
  }

  void Visit(const FunctionLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("FunctionLiteral"));
    if (literal->name()) {
      Visit(literal->name());
      rb_hash_aset(hash, SYM("name"), ret_);
    }
    {
      VALUE array = rb_ary_new();
      for (Identifiers::const_iterator it = literal->params().begin(),
           last = literal->params().end(); it != last; ++it) {
        (*it)->Accept(this);
        rb_ary_push(array, ret_);
      }
      rb_hash_aset(hash, SYM("params"), array);
    }
    {
      VALUE array = rb_ary_new();
      for (Statements::const_iterator it = literal->body().begin(),
           last = literal->body().end(); it != last; ++it) {
        (*it)->Accept(this);
        rb_ary_push(array, ret_);
      }
      rb_hash_aset(hash, SYM("body"), array);
    }
    ret_ = hash;
  }

  void Visit(const IndexAccess* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("IndexAccess"));
    expr->target()->Accept(this);
    rb_hash_aset(hash, SYM("target"), ret_);
    expr->key()->Accept(this);
    rb_hash_aset(hash, SYM("key"), ret_);
    ret_ = hash;
  }

  void Visit(const IdentifierAccess* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("IdentifierAccess"));
    expr->target()->Accept(this);
    rb_hash_aset(hash, SYM("target"), ret_);
    Visit(expr->key());
    rb_hash_aset(hash, SYM("key"), ret_);
    ret_ = hash;
  }

  void Visit(const FunctionCall* call) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("FunctionCall"));
    call->target()->Accept(this);
    rb_hash_aset(hash, SYM("target"), ret_);
    VALUE args = rb_ary_new();
    for (Expressions::const_iterator it = call->args().begin(),
         last = call->args().end(); it != last; ++it) {
      (*it)->Accept(this);
      rb_ary_push(args, ret_);
    }
    rb_hash_aset(hash, SYM("args"), args);
    ret_ = hash;
  }

  void Visit(const ConstructorCall* call) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), rb_str_new_cstr("ConstructorCall"));
    call->target()->Accept(this);
    rb_hash_aset(hash, SYM("target"), ret_);
    VALUE args = rb_ary_new();
    for (Expressions::const_iterator it = call->args().begin(),
         last = call->args().end(); it != last; ++it) {
      (*it)->Accept(this);
      rb_ary_push(args, ret_);
    }
    rb_hash_aset(hash, SYM("args"), args);
    ret_ = hash;
  }

 private:
  VALUE ret_;
};

} }  // namespace iv::phonic

#undef SYM
#endif  // _IV_PHONIC_CREATOR_H_
