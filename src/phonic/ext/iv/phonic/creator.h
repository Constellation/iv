#ifndef _IV_PHONIC_CREATOR_H_
#define _IV_PHONIC_CREATOR_H_
extern "C" {
#include <ruby.h>
}
#include <iv/ast_visitor.h>
#include <iv/utils.h>
#include <iv/maybe.h>
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
    rb_hash_aset(hash, SYM("type"), SYM("Block"));
    VALUE array = rb_ary_new();
    for (Statements::const_iterator it = stmt->body().begin(),
         last = stmt->body().end(); it != last; ++it) {
      (*it)->Accept(this);
      rb_ary_push(array, ret_);
    }
    rb_hash_aset(hash, SYM("body"), array);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const FunctionStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("FunctionStatement"));
    Visit(stmt->function());
    rb_hash_aset(hash, SYM("body"), ret_);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const FunctionDeclaration* decl) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("FunctionDeclaration"));
    Visit(decl->function());
    rb_hash_aset(hash, SYM("body"), ret_);
    SetLocation(hash, decl);
    ret_ = hash;
  }

  void Visit(const VariableStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("VariableStatement"));
    rb_hash_aset(hash, SYM("const"), stmt->IsConst() ? Qtrue : Qfalse);
    VALUE array = rb_ary_new();
    for (Declarations::const_iterator it = stmt->decls().begin(),
         last = stmt->decls().end(); it != last; ++it) {
      Visit(*it);
      rb_ary_push(array, ret_);
    }
    rb_hash_aset(hash, SYM("body"), array);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const Declaration* decl) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("Declaration"));
    Visit(decl->name());
    rb_hash_aset(hash, SYM("name"), ret_);
    if (const core::Maybe<const Expression> expr = decl->expr()) {
      (*expr).Accept(this);
      rb_hash_aset(hash, SYM("expr"), ret_);
    }
    SetLocation(hash, decl);
    ret_ = hash;
  }

  void Visit(const EmptyStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("EmptyStatement"));
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const IfStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("IfStatement"));
    stmt->cond()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    stmt->then_statement()->Accept(this);
    rb_hash_aset(hash, SYM("then"), ret_);
    if (const core::Maybe<const Statement> else_stmt = stmt->else_statement()) {
      (*else_stmt).Accept(this);
      rb_hash_aset(hash, SYM("else"), ret_);
    }
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const DoWhileStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("DoWhileStatement"));
    stmt->cond()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const WhileStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("WhileStatement"));
    stmt->cond()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const ForStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ForStatement"));
    if (const core::Maybe<const Statement> init = stmt->init()) {
      (*init).Accept(this);
      rb_hash_aset(hash, SYM("init"), ret_);
    }
    if (const core::Maybe<const Expression> cond = stmt->cond()) {
      (*cond).Accept(this);
      rb_hash_aset(hash, SYM("cond"), ret_);
    }
    if (const core::Maybe<const Statement> next = stmt->next()) {
      (*next).Accept(this);
      rb_hash_aset(hash, SYM("next"), ret_);
    }
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const ForInStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ForInStatement"));
    stmt->each()->Accept(this);
    rb_hash_aset(hash, SYM("each"), ret_);
    stmt->enumerable()->Accept(this);
    rb_hash_aset(hash, SYM("enum"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const ContinueStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ContinueStatement"));
    if (const core::Maybe<const Identifier> label = stmt->label()) {
      (*label).Accept(this);
      rb_hash_aset(hash, SYM("label"), ret_);
    }
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const BreakStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("BreakStatement"));
    if (const core::Maybe<const Identifier> label = stmt->label()) {
      (*label).Accept(this);
      rb_hash_aset(hash, SYM("label"), ret_);
    }
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const ReturnStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ReturnStatement"));
    if (const core::Maybe<const Expression> expr = stmt->expr()) {
      (*expr).Accept(this);
      rb_hash_aset(hash, SYM("expr"), ret_);
    }
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const WithStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("WithStatement"));
    stmt->context()->Accept(this);
    rb_hash_aset(hash, SYM("context"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const LabelledStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("LabelledStatement"));
    stmt->label()->Accept(this);
    rb_hash_aset(hash, SYM("label"), ret_);
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const SwitchStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("SwitchStatement"));
    stmt->expr()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    VALUE array = rb_ary_new();
    for (CaseClauses::const_iterator it = stmt->clauses().begin(),
         last = stmt->clauses().end(); it != last; ++it) {
      Visit(*it);
      rb_ary_push(array, ret_);
    }
    rb_hash_aset(hash, SYM("clauses"), array);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const CaseClause* cl) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("CaseClause"));
    if (const core::Maybe<const Expression> expr = cl->expr()) {
      rb_hash_aset(hash, SYM("kind"), SYM("Case"));
      (*expr).Accept(this);
      rb_hash_aset(hash, SYM("expr"), ret_);
    } else {
      rb_hash_aset(hash, SYM("kind"), SYM("Default"));
    }
    VALUE stmts = rb_ary_new();
    for (Statements::const_iterator st = cl->body().begin(),
         stlast = cl->body().end(); st != stlast; ++st) {
      (*st)->Accept(this);
      rb_ary_push(stmts, ret_);
    }
    rb_hash_aset(hash, SYM("body"), stmts);
    SetLocation(hash, cl);
    ret_ = hash;
  }

  void Visit(const ThrowStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ThrowStatement"));
    stmt->expr()->Accept(this);
    rb_hash_aset(hash, SYM("expr"), ret_);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const TryStatement* stmt) {
    assert(stmt->catch_block() || stmt->finally_block());
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("TryStatement"));
    stmt->body()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    if (const core::Maybe<const Identifier> ident = stmt->catch_name()) {
      Visit(ident.Address());
      rb_hash_aset(hash, SYM("catch_name"), ret_);
      Visit(stmt->catch_block().Address());
      rb_hash_aset(hash, SYM("catch_block"), ret_);
    }
    if (const core::Maybe<const Block> block = stmt->finally_block()) {
      Visit(block.Address());
      rb_hash_aset(hash, SYM("finally_block"), ret_);
    }
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const DebuggerStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("DebuggerStatement"));
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const ExpressionStatement* stmt) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ExpressionStatement"));
    stmt->expr()->Accept(this);
    rb_hash_aset(hash, SYM("body"), ret_);
    SetLocation(hash, stmt);
    ret_ = hash;
  }

  void Visit(const Assignment* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("Assignment"));
    rb_hash_aset(hash, SYM("op"),
                 rb_str_new_cstr(core::Token::ToString(expr->op())));
    expr->left()->Accept(this);
    rb_hash_aset(hash, SYM("left"), ret_);
    expr->right()->Accept(this);
    rb_hash_aset(hash, SYM("right"), ret_);
    SetLocation(hash, expr);
    ret_ = hash;
  }

  void Visit(const BinaryOperation* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("BinaryOperation"));
    rb_hash_aset(hash, SYM("op"),
                 rb_str_new_cstr(core::Token::ToString(expr->op())));
    expr->left()->Accept(this);
    rb_hash_aset(hash, SYM("left"), ret_);
    expr->right()->Accept(this);
    rb_hash_aset(hash, SYM("right"), ret_);
    SetLocation(hash, expr);
    ret_ = hash;
  }

  void Visit(const ConditionalExpression* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ConditionalExpression"));
    expr->cond()->Accept(this);
    rb_hash_aset(hash, SYM("cond"), ret_);
    expr->left()->Accept(this);
    rb_hash_aset(hash, SYM("left"), ret_);
    expr->right()->Accept(this);
    rb_hash_aset(hash, SYM("right"), ret_);
    SetLocation(hash, expr);
    ret_ = hash;
  }

  void Visit(const UnaryOperation* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("UnaryOperation"));
    rb_hash_aset(hash, SYM("op"),
                 rb_str_new_cstr(core::Token::ToString(expr->op())));
    expr->expr()->Accept(this);
    rb_hash_aset(hash, SYM("expr"), ret_);
    SetLocation(hash, expr);
    ret_ = hash;
  }

  void Visit(const PostfixExpression* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("PostfixExpression"));
    rb_hash_aset(hash, SYM("op"),
                 rb_str_new_cstr(core::Token::ToString(expr->op())));
    expr->expr()->Accept(this);
    rb_hash_aset(hash, SYM("expr"), ret_);
    SetLocation(hash, expr);
    ret_ = hash;
  }

  void Visit(const StringLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("StringLiteral"));
    const StringLiteral::value_type& str = literal->value();
    rb_hash_aset(hash, SYM("value"),
                 Encoding::ConvertToDefaultInternal(
                     rb_enc_str_new(
                         reinterpret_cast<const char*>(str.data()),
                         str.size()*2,
                         rb_to_encoding(Encoding::UTF16Encoding()))));
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const NumberLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("NumberLiteral"));
    rb_hash_aset(hash, SYM("value"), rb_float_new(literal->value()));
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const Identifier* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("Identifier"));
    const Identifier::value_type& str = literal->value();
    if (literal->type() == core::Token::IDENTIFIER) {
      rb_hash_aset(hash, SYM("kind"), SYM("Identifier"));
    } else if (literal->type() == core::Token::NUMBER) {
      rb_hash_aset(hash, SYM("kind"), SYM("Number"));
    } else {
      rb_hash_aset(hash, SYM("kind"), SYM("String"));
    }
    rb_hash_aset(hash, SYM("value"),
                 Encoding::ConvertToDefaultInternal(
                     rb_enc_str_new(
                         reinterpret_cast<const char*>(str.data()),
                         str.size()*2,
                         rb_to_encoding(Encoding::UTF16Encoding()))));
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const ThisLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ThisLiteral"));
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const NullLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("NullLiteral"));
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const TrueLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("TrueLiteral"));
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const FalseLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("FalseLiteral"));
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const RegExpLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("RegExpLiteral"));
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
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const ArrayLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ArrayLiteral"));
    VALUE array = rb_ary_new();
    for (MaybeExpressions::const_iterator it = literal->items().begin(),
         last = literal->items().end(); it != last; ++it) {
      if (*it) {
        (**it).Accept(this);
        rb_ary_push(array, ret_);
      } else {
        rb_ary_push(array, NewArrayHole());
      }
    }
    rb_hash_aset(hash, SYM("value"), array);
    SetLocation(hash, literal);
    ret_ = hash;
  }

  static VALUE NewArrayHole() {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ArrayHole"));
    return hash;
  }

  void Visit(const ObjectLiteral* literal) {
    using std::tr1::get;
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ObjectLiteral"));
    VALUE array = rb_ary_new();
    for (ObjectLiteral::Properties::const_iterator it = literal->properties().begin(),
         last = literal->properties().end(); it != last; ++it) {
      VALUE item = rb_hash_new();
      switch (get<0>(*it)) {
        case ObjectLiteral::DATA:
          rb_hash_aset(item, SYM("kind"), SYM("Data"));
          break;

        case ObjectLiteral::SET:
          rb_hash_aset(item, SYM("kind"), SYM("Setter"));
          break;

        case ObjectLiteral::GET:
          rb_hash_aset(item, SYM("kind"), SYM("Getter"));
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
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const FunctionLiteral* literal) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("FunctionLiteral"));
    if (const core::Maybe<const Identifier> name = literal->name()) {
      Visit(name.Address());
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
    SetLocation(hash, literal);
    ret_ = hash;
  }

  void Visit(const IndexAccess* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("IndexAccess"));
    expr->target()->Accept(this);
    rb_hash_aset(hash, SYM("target"), ret_);
    expr->key()->Accept(this);
    rb_hash_aset(hash, SYM("key"), ret_);
    SetLocation(hash, expr);
    ret_ = hash;
  }

  void Visit(const IdentifierAccess* expr) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("IdentifierAccess"));
    expr->target()->Accept(this);
    rb_hash_aset(hash, SYM("target"), ret_);
    Visit(expr->key());
    rb_hash_aset(hash, SYM("key"), ret_);
    SetLocation(hash, expr);
    ret_ = hash;
  }

  void Visit(const FunctionCall* call) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("FunctionCall"));
    call->target()->Accept(this);
    rb_hash_aset(hash, SYM("target"), ret_);
    VALUE args = rb_ary_new();
    for (Expressions::const_iterator it = call->args().begin(),
         last = call->args().end(); it != last; ++it) {
      (*it)->Accept(this);
      rb_ary_push(args, ret_);
    }
    rb_hash_aset(hash, SYM("args"), args);
    SetLocation(hash, call);
    ret_ = hash;
  }

  void Visit(const ConstructorCall* call) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, SYM("type"), SYM("ConstructorCall"));
    call->target()->Accept(this);
    rb_hash_aset(hash, SYM("target"), ret_);
    VALUE args = rb_ary_new();
    for (Expressions::const_iterator it = call->args().begin(),
         last = call->args().end(); it != last; ++it) {
      (*it)->Accept(this);
      rb_ary_push(args, ret_);
    }
    rb_hash_aset(hash, SYM("args"), args);
    SetLocation(hash, call);
    ret_ = hash;
  }

 private:

  static void SetLocation(VALUE hash, const AstNode* node) {
    rb_hash_aset(hash, SYM("begin"), INT2NUM(node->begin_position()));
    rb_hash_aset(hash, SYM("end"), INT2NUM(node->end_position()));
  }

  VALUE ret_;
};

} }  // namespace iv::phonic

#undef SYM
#endif  // _IV_PHONIC_CREATOR_H_
