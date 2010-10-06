#include <cstdio>
#include <cassert>
#include <cmath>
#include <iostream>  // NOLINT
#include <tr1/tuple>
#include <tr1/array>
#include <boost/foreach.hpp>
#include "token.h"
#include "size_t.h"
#include "interpreter.h"
#include "scope.h"
#include "jsreference.h"
#include "jsobject.h"
#include "jsstring.h"
#include "jsfunction.h"
#include "jsregexp.h"
#include "jsexception.h"
#include "jsproperty.h"
#include "jsenv.h"
#include "jsarray.h"
#include "ustream.h"

namespace {

const std::string eval_string("eval");
const std::string arguments_string("arguments");

}  // namespace

namespace iv {
namespace lv5 {

#define CHECK  context_->error());\
  if (context_->IsError()) {\
    return;\
  }\
  ((void)0


#define CHECK_WITH(val) context_->error());\
  if (context_->IsError()) {\
    return val;\
  }\
  ((void)0


#define CHECK_TO_WITH(error, val) error);\
  if (*error) {\
    return val;\
  }\
  ((void)0


#define RETURN_STMT(type, val, target)\
  do {\
    context_->SetStatement(type, val, target);\
    return;\
  } while (0)


#define ABRUPT()\
  do {\
    return;\
  } while (0)


#define EVAL(node)\
  node->Accept(this);\
  if (context_->IsError()) {\
    return;\
  }


Interpreter::Interpreter()
  : context_(NULL) {
}


Interpreter::~Interpreter() {
}


// section 13.2.1 [[Call]]
void Interpreter::CallCode(
    const JSCodeFunction& code,
    const Arguments& args,
    JSErrorCode::Type* error) {
  // step 1
  JSVal this_value = args.this_binding();
  if (this_value.IsUndefined()) {
    this_value.set_value(context_->global_obj());
  } else if (!this_value.IsObject()) {
    JSObject* obj = this_value.ToObject(context_, CHECK);
    this_value.set_value(obj);
  }
  JSDeclEnv* local_env = NewDeclarativeEnvironment(context_, code.scope());
  ContextSwitcher switcher(context_, local_env,
                           local_env, this_value.object(),
                           code.code()->strict());

  // section 10.5 Declaration Binding Instantiation
  const core::Scope* const scope = code.code()->scope();
  // step 1
  JSEnv* const env = context_->variable_env();
  // step 2
  const bool configurable_bindings = false;
  // step 4
  {
    Arguments::JSVals arguments = args.args();
    std::size_t arg_count = arguments.size();
    std::size_t n = 0;
    BOOST_FOREACH(core::Identifier* const ident,
                  code.code()->params()) {
      ++n;
      Symbol arg_name = context_->Intern(ident->value());
      if (!env->HasBinding(arg_name)) {
        env->CreateMutableBinding(context_, arg_name, configurable_bindings);
      }
      if (n > arg_count) {
        env->SetMutableBinding(context_, arg_name,
                               JSVal::Undefined(), context_->IsStrict(), CHECK);
      } else {
        env->SetMutableBinding(context_, arg_name,
                               arguments[n], context_->IsStrict(), CHECK);
      }
    }
  }
  BOOST_FOREACH(core::FunctionLiteral* const f,
                scope->function_declarations()) {
    const Symbol fn = context_->Intern(f->name()->value());
    EVAL(f);
    JSVal fo = context_->ret();
    if (!env->HasBinding(fn)) {
      env->CreateMutableBinding(context_, fn, configurable_bindings);
    }
    env->SetMutableBinding(context_, fn, fo, context_->IsStrict(), CHECK);
  }
  BOOST_FOREACH(const core::Scope::Variable& var, scope->variables()) {
    const Symbol dn = context_->Intern(var.first->value());
    if (!env->HasBinding(dn)) {
      env->CreateMutableBinding(context_, dn, configurable_bindings);
      env->SetMutableBinding(context_, dn,
                             JSVal::Undefined(), context_->IsStrict(), CHECK);
    }
  }

  JSVal value;
  BOOST_FOREACH(core::Statement* const stmt, code.code()->body()) {
    EVAL(stmt);
    if (context_->IsMode<Context::THROW>()) {
      // section 12.1 step 4
      // TODO(Constellation) value to exception
      RETURN_STMT(Context::THROW, value, NULL);
      return;
    }
    if (!context_->ret().IsUndefined()) {
      value = context_->ret();
    }
    if (!context_->IsMode<Context::NORMAL>()) {
      ABRUPT();
    }
  }
}


void Interpreter::Run(core::FunctionLiteral* global) {
  // section 10.5 Declaration Binding Instantiation
  const bool configurable_bindings = false;
  const core::Scope* const scope = global->scope();
  JSEnv* const env = context_->variable_env();
  StrictSwitcher switcher(context_, global->strict());
  BOOST_FOREACH(core::FunctionLiteral* const f,
                scope->function_declarations()) {
    const Symbol fn = context_->Intern(f->name()->value());
    EVAL(f);
    JSVal fo = context_->ret();
    if (!env->HasBinding(fn)) {
      env->CreateMutableBinding(context_, fn, configurable_bindings);
    }
    env->SetMutableBinding(context_, fn, fo, context_->IsStrict(), CHECK);
  }

  BOOST_FOREACH(const core::Scope::Variable& var, scope->variables()) {
    const Symbol dn = context_->Intern(var.first->value());
    if (!env->HasBinding(dn)) {
      env->CreateMutableBinding(context_, dn, configurable_bindings);
      env->SetMutableBinding(context_, dn,
                             JSVal::Undefined(), context_->IsStrict(), CHECK);
    }
  }

  JSVal value;
  // section 14 Program
  BOOST_FOREACH(core::Statement* const stmt, global->body()) {
    EVAL(stmt);
    if (context_->IsMode<Context::THROW>()) {
      // section 12.1 step 4
      // TODO(Constellation) value to exception
      RETURN_STMT(Context::THROW, value, NULL);
      return;
    }
    if (!context_->ret().IsUndefined()) {
      value = context_->ret();
    }
    if (!context_->IsMode<Context::NORMAL>()) {
      ABRUPT();
    }
  }
  return;
}


void Interpreter::Visit(core::Block* block) {
  // section 12.1 Block
  context_->set_mode(Context::Context::NORMAL);
  JSVal value;
  BOOST_FOREACH(core::Statement* const stmt, block->body()) {
    EVAL(stmt);
    if (context_->IsMode<Context::THROW>()) {
      // section 12.1 step 4
      // TODO(Constellation) value to exception
      RETURN_STMT(Context::THROW, value, NULL);
      return;
    }
    if (!context_->ret().IsUndefined()) {
      value = context_->ret();
    }

    if (context_->IsMode<Context::BREAK>() &&
        context_->InCurrentLabelSet(block)) {
      RETURN_STMT(Context::NORMAL, value, NULL);
    }
    if (!context_->IsMode<Context::NORMAL>()) {
      ABRUPT();
    }
  }
  context_->set_ret(value);
}


void Interpreter::Visit(core::FunctionStatement* stmt) {
  core::FunctionLiteral* const func = stmt->function();
  func->name()->Accept(this);
  const JSVal lhs = context_->ret();
  EVAL(func);
  const JSVal val = GetValue(context_->ret(), CHECK);
  PutValue(lhs, val, CHECK);
}


void Interpreter::Visit(core::VariableStatement* var) {
  // bool is_const = var->IsConst();
  BOOST_FOREACH(const core::Declaration* const decl, var->decls()) {
    EVAL(decl->name());
    const JSVal lhs = context_->ret();
    if (decl->expr()) {
      EVAL(decl->expr());
      const JSVal val = GetValue(context_->ret(), CHECK);
      PutValue(lhs, val, CHECK);
      // TODO(Constellation) 12.2 step 5 Return a String value
    }
  }
}


void Interpreter::Visit(core::Declaration* decl) {
  UNREACHABLE();
}


void Interpreter::Visit(core::EmptyStatement* empty) {
  RETURN_STMT(Context::NORMAL, JSVal::Undefined(), NULL);
}


void Interpreter::Visit(core::IfStatement* stmt) {
  EVAL(stmt->cond());
  const JSVal expr = GetValue(context_->ret(), CHECK);
  const bool val = expr.ToBoolean(CHECK);
  if (val) {
    EVAL(stmt->then_statement());
    return;
  } else {
    core::Statement* const else_stmt = stmt->else_statement();
    if (else_stmt) {
      EVAL(else_stmt);
      return;
    } else {
      RETURN_STMT(Context::NORMAL, JSVal::Undefined(), NULL);
    }
  }
}


void Interpreter::Visit(core::DoWhileStatement* stmt) {
  JSVal value;
  bool iterating = true;
  while (iterating) {
    EVAL(stmt->body());
    if (!context_->ret().IsUndefined()) {
      value = context_->ret();
    }
    if (!context_->IsMode<Context::CONTINUE>() ||
        !context_->InCurrentLabelSet(stmt)) {
      if (context_->IsMode<Context::BREAK>() &&
          context_->InCurrentLabelSet(stmt)) {
        RETURN_STMT(Context::NORMAL, value, NULL);
      }
      if (!context_->IsMode<Context::NORMAL>()) {
        ABRUPT();
      }
    }
    EVAL(stmt->cond());
    const JSVal expr = GetValue(context_->ret(), CHECK);
    const bool val = expr.ToBoolean(CHECK);
    iterating = val;
  }
  RETURN_STMT(Context::NORMAL, value, NULL);
}


void Interpreter::Visit(core::WhileStatement* stmt) {
  JSVal value;
  while (true) {
    EVAL(stmt->cond());
    const JSVal expr = GetValue(context_->ret(), CHECK);
    const bool val = expr.ToBoolean(CHECK);
    if (val) {
      EVAL(stmt->body());
      if (!context_->ret().IsUndefined()) {
        value = context_->ret();
      }
      if (!context_->IsMode<Context::CONTINUE>() ||
          !context_->InCurrentLabelSet(stmt)) {
        if (context_->IsMode<Context::BREAK>() &&
            context_->InCurrentLabelSet(stmt)) {
          RETURN_STMT(Context::NORMAL, value, NULL);
        }
        if (!context_->IsMode<Context::NORMAL>()) {
          ABRUPT();
        }
      }
    } else {
      RETURN_STMT(Context::NORMAL, value, NULL);
    }
  }
}


void Interpreter::Visit(core::ForStatement* stmt) {
  if (stmt->init()) {
    EVAL(stmt->init());
    GetValue(context_->ret(), CHECK);
  }
  JSVal value;
  while (true) {
    if (stmt->cond()) {
      EVAL(stmt->cond());
      const JSVal expr = GetValue(context_->ret(), CHECK);
      const bool val = expr.ToBoolean(CHECK);
      if (!val) {
        RETURN_STMT(Context::NORMAL, value, NULL);
      }
    }
    EVAL(stmt->body());
    if (!context_->ret().IsUndefined()) {
      value = context_->ret();
    }
    if (!context_->IsMode<Context::CONTINUE>() ||
        !context_->InCurrentLabelSet(stmt)) {
      if (context_->IsMode<Context::BREAK>() &&
          context_->InCurrentLabelSet(stmt)) {
        RETURN_STMT(Context::NORMAL, value, NULL);
      }
      if (!context_->IsMode<Context::NORMAL>()) {
        ABRUPT();
      }
    }
    if (stmt->next()) {
      EVAL(stmt->next());
      GetValue(context_->ret(), CHECK);
    }
  }
}


void Interpreter::Visit(core::ForInStatement* stmt) {
  EVAL(stmt->enumerable());
  JSVal expr = GetValue(context_->ret(), CHECK);
  if (expr.IsNull() || expr.IsUndefined()) {
    RETURN_STMT(Context::NORMAL, JSVal::Undefined(), NULL);
  }
  JSObject* const obj = expr.ToObject(context_, CHECK);
  JSVal value;
  JSObject* current = obj;
  do {
    BOOST_FOREACH(const JSObject::Properties::value_type& set,
                  current->table()) {
      if (!set.second->IsEnumerable()) {
        continue;
      }
      JSVal rhs(context_->ToString(set.first));
      EVAL(stmt->each());
      if (stmt->each()->AsVariableStatement()) {
        core::Identifier* ident =
            stmt->each()->AsVariableStatement()->decls()[0]->name();
        EVAL(ident);
      }
      JSVal lhs = context_->ret();
      PutValue(lhs, rhs, CHECK);
      EVAL(stmt->body());
      if (!context_->ret().IsUndefined()) {
        value = context_->ret();
      }
      if (!context_->IsMode<Context::CONTINUE>() ||
          !context_->InCurrentLabelSet(stmt)) {
        if (context_->IsMode<Context::BREAK>() &&
            context_->InCurrentLabelSet(stmt)) {
          RETURN_STMT(Context::NORMAL, value, NULL);
        }
        if (!context_->IsMode<Context::NORMAL>()) {
          ABRUPT();
        }
      }
    }
    current = current->prototype();
  } while (current);
  RETURN_STMT(Context::NORMAL, value, NULL);
}


void Interpreter::Visit(core::ContinueStatement* stmt) {
  RETURN_STMT(Context::CONTINUE, JSVal::Undefined(), stmt->target());
}


void Interpreter::Visit(core::BreakStatement* stmt) {
  if (stmt->target()) {
  } else {
    if (stmt->label()) {
      RETURN_STMT(Context::NORMAL, JSVal::Undefined(), NULL);
    }
  }
  RETURN_STMT(Context::BREAK, JSVal::Undefined(), stmt->target());
}


void Interpreter::Visit(core::ReturnStatement* stmt) {
  if (stmt->expr()) {
    EVAL(stmt->expr());
    const JSVal value = GetValue(context_->ret(), CHECK);
    RETURN_STMT(Context::RETURN, value, NULL);
  } else {
    RETURN_STMT(Context::RETURN, JSVal::Undefined(), NULL);
  }
}


// section 12.10 The with Statement
void Interpreter::Visit(core::WithStatement* stmt) {
  EVAL(stmt->context());
  const JSVal val = GetValue(context_->ret(), CHECK);
  JSObject* const obj = val.ToObject(context_, CHECK);
  JSEnv* const old_env = context_->lexical_env();
  JSObjectEnv* const new_env = NewObjectEnvironment(context_, obj, old_env);
  new_env->set_provide_this(true);
  {
    LexicalEnvSwitcher switcher(context_, new_env);
    EVAL(stmt->body());
  }
}


void Interpreter::Visit(core::LabelledStatement* stmt) {
  EVAL(stmt->body());
}


void Interpreter::Visit(core::CaseClause* clause) {
  UNREACHABLE();
}


void Interpreter::Visit(core::SwitchStatement* stmt) {
  EVAL(stmt->expr());
  const JSVal cond = GetValue(context_->ret(), CHECK);
  // Case Block
  JSVal value;
  {
    typedef core::SwitchStatement::CaseClauses CaseClauses;
    using core::CaseClause;
    bool found = false;
    bool default_found = false;
    bool finalize = false;
    std::size_t default_index = 0;
    const CaseClauses& clauses = stmt->clauses();
    const std::size_t len = clauses.size();
    for (std::size_t i = 0; i < len; ++i) {
      const CaseClause* const clause = clauses[i];
      if (clause->IsDefault()) {
        default_index = i;
        default_found = true;
      } else {
        if (!found) {
          EVAL(clause->expr());
          const JSVal res = GetValue(context_->ret(), CHECK);
          if (StrictEqual(cond, res)) {
            found = true;
          }
        }
        // case's fall through
        if (found) {
          BOOST_FOREACH(core::Statement* const st, clause->body()) {
            EVAL(st);
            if (!context_->ret().IsUndefined()) {
              value = context_->ret();
            }
            if (!context_->IsMode<Context::NORMAL>()) {
              context_->ret() = value;
              finalize = true;
              break;
            }
          }
          if (finalize) {
            break;
          }
        }
      }
    }
    if (!finalize && !found && default_found) {
      for (std::size_t i = default_index; i < len; ++i) {
        const CaseClause* const clause = clauses[i];
        BOOST_FOREACH(core::Statement* const st, clause->body()) {
          EVAL(st);
          if (!context_->ret().IsUndefined()) {
            value = context_->ret();
          }
          if (!context_->IsMode<Context::NORMAL>()) {
            context_->ret() = value;
            break;
          }
        }
        if (finalize) {
          break;
        }
      }
    }
  }

  if (context_->IsMode<Context::BREAK>() && context_->InCurrentLabelSet(stmt)) {
    RETURN_STMT(Context::NORMAL, value, NULL);
  }
}


// section 12.13 The throw Statement
void Interpreter::Visit(core::ThrowStatement* stmt) {
  EVAL(stmt->expr());
  JSVal ref = GetValue(context_->ret(), CHECK);
  RETURN_STMT(Context::THROW, ref, NULL);
}


// section 12.14 The try Statement
void Interpreter::Visit(core::TryStatement* stmt) {
  stmt->body()->Accept(this);
  if (context_->IsMode<Context::THROW>() || context_->IsError()) {
    if (stmt->catch_block()) {
      context_->set_mode(Context::NORMAL);
      context_->set_error(JSErrorCode::Normal);
      JSEnv* const old_env = context_->lexical_env();
      JSEnv* const catch_env = NewDeclarativeEnvironment(context_, old_env);
      const Symbol name = context_->Intern(stmt->catch_name()->value());
      const JSVal ex = (context_->IsMode<Context::THROW>()) ?
          context_->ret() : JSVal::Undefined();
      catch_env->CreateMutableBinding(context_, name, false);
      catch_env->SetMutableBinding(context_, name, ex, false, CHECK);
      {
        LexicalEnvSwitcher switcher(context_, catch_env);
        EVAL(stmt->catch_block());
      }
    }
  }
  const Context::Mode mode = context_->mode();
  const JSVal value = context_->ret();
  core::BreakableStatement* const target = context_->target();

  context_->set_error(JSErrorCode::Normal);
  context_->SetStatement(Context::Context::NORMAL, JSVal::Undefined(), NULL);

  if (stmt->finally_block()) {
    stmt->finally_block()->Accept(this);
    if (context_->IsMode<Context::NORMAL>()) {
      RETURN_STMT(mode, value, target);
    }
  }
}


void Interpreter::Visit(core::DebuggerStatement* stmt) {
  // section 12.15 debugger statement
  // implementation define debugging facility is not available
  RETURN_STMT(Context::NORMAL, JSVal::Undefined(), NULL);
}


void Interpreter::Visit(core::ExpressionStatement* stmt) {
  EVAL(stmt->expr());
  const JSVal value = GetValue(context_->ret(), CHECK);
  RETURN_STMT(Context::NORMAL, value, NULL);
}


void Interpreter::Visit(core::Assignment* assign) {
  using core::Token;
  EVAL(assign->left());
  const JSVal lref(context_->ret());
  const JSVal lhs = GetValue(lref, CHECK);
  EVAL(assign->right());
  const JSVal rhs = GetValue(context_->ret(), CHECK);
  JSVal result;
  switch (assign->op()) {
    case Token::ASSIGN: {  // =
      result = rhs;
      break;
    }
    case Token::ASSIGN_ADD: {  // +=
      const JSVal lprim = lhs.ToPrimitive(context_, JSObject::NONE, CHECK);
      const JSVal rprim = rhs.ToPrimitive(context_, JSObject::NONE, CHECK);
      if (lprim.IsString() || rprim.IsString()) {
        const JSString* const lstr = lprim.ToString(context_, CHECK);
        const JSString* const rstr = rprim.ToString(context_, CHECK);
        context_->ret().set_value(JSString::New(context_, *lstr + *rstr));
        return;
      }
      assert(lprim.IsNumber() && rprim.IsNumber());
      const double left_num = lprim.ToNumber(context_, CHECK);
      const double right_num = rprim.ToNumber(context_, CHECK);
      result.set_value(left_num + right_num);
      break;
    }
    case Token::ASSIGN_SUB: {  // -=
      const double left_num = lhs.ToNumber(context_, CHECK);
      const double right_num = rhs.ToNumber(context_, CHECK);
      result.set_value(left_num - right_num);
      break;
    }
    case Token::ASSIGN_MUL: {  // *=
      const double left_num = lhs.ToNumber(context_, CHECK);
      const double right_num = rhs.ToNumber(context_, CHECK);
      result.set_value(left_num * right_num);
      break;
    }
    case Token::ASSIGN_MOD: {  // %=
      const double left_num = lhs.ToNumber(context_, CHECK);
      const double right_num = rhs.ToNumber(context_, CHECK);
      result.set_value(std::fmod(left_num, right_num));
      break;
    }
    case Token::ASSIGN_DIV: {  // /=
      const double left_num = lhs.ToNumber(context_, CHECK);
      const double right_num = rhs.ToNumber(context_, CHECK);
      result.set_value(left_num / right_num);
      break;
    }
    case Token::ASSIGN_SAR: {  // >>=
      const double left_num = lhs.ToNumber(context_, CHECK);
      const double right_num = rhs.ToNumber(context_, CHECK);
      result.set_value(
          static_cast<double>(core::Conv::DoubleToInt32(left_num)
                 >> (core::Conv::DoubleToInt32(right_num) & 0x1f)));
      break;
    }
    case Token::ASSIGN_SHR: {  // >>>=
      const double left_num = lhs.ToNumber(context_, CHECK);
      const double right_num = rhs.ToNumber(context_, CHECK);
      result.set_value(
          static_cast<double>(core::Conv::DoubleToUInt32(left_num)
                 >> (core::Conv::DoubleToInt32(right_num) & 0x1f)));
      break;
    }
    case Token::ASSIGN_SHL: {  // <<=
      const double left_num = lhs.ToNumber(context_, CHECK);
      const double right_num = rhs.ToNumber(context_, CHECK);
      result.set_value(
          static_cast<double>(core::Conv::DoubleToInt32(left_num)
                 << (core::Conv::DoubleToInt32(right_num) & 0x1f)));
      break;
    }
    case Token::ASSIGN_BIT_AND: {  // &=
      const double left_num = lhs.ToNumber(context_, CHECK);
      const double right_num = rhs.ToNumber(context_, CHECK);
      result.set_value(
          static_cast<double>(core::Conv::DoubleToInt32(left_num)
                 & (core::Conv::DoubleToInt32(right_num))));
      break;
    }
    case Token::ASSIGN_BIT_OR: {  // |=
      const double left_num = lhs.ToNumber(context_, CHECK);
      const double right_num = rhs.ToNumber(context_, CHECK);
      result.set_value(
          static_cast<double>(core::Conv::DoubleToInt32(left_num)
                 | (core::Conv::DoubleToInt32(right_num))));
      break;
    }
    default: {
      UNREACHABLE();
      break;
    }
  }
  if (lref.IsReference()) {
    const JSReference* const ref = lref.reference();
    if (ref->IsStrictReference() &&
        ref->base()->IsEnvironment()) {
      const Symbol sym = ref->GetReferencedName();
      if (sym == context_->Intern(eval_string) ||
          sym == context_->Intern(arguments_string)) {
        context_->set_error(JSErrorCode::SyntaxError);
        return;
      }
    }
  }
  PutValue(lref, result, CHECK);
  context_->set_ret(result);
}


void Interpreter::Visit(core::BinaryOperation* binary) {
  using core::Token;
  const Token::Type token = binary->op();
  EVAL(binary->left());
  const JSVal lhs = GetValue(context_->ret(), CHECK);
  {
    switch (token) {
      case Token::LOGICAL_AND: {  // &&
        const bool cond = lhs.ToBoolean(CHECK);
        if (!cond) {
          context_->set_ret(lhs);
          return;
        } else {
          EVAL(binary->right());
          context_->ret() = GetValue(context_->ret(), CHECK);
          return;
        }
      }

      case Token::LOGICAL_OR: {  // ||
        const bool cond = lhs.ToBoolean(CHECK);
        if (cond) {
          context_->set_ret(lhs);
          return;
        } else {
          EVAL(binary->right());
          context_->ret() = GetValue(context_->ret(), CHECK);
          return;
        }
      }

      default:
        break;
        // pass
    }
  }

  {
    EVAL(binary->right());
    const JSVal rhs = GetValue(context_->ret(), CHECK);
    switch (token) {
      case Token::MUL: {  // *
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(left_num * right_num);
        return;
      }

      case Token::DIV: {  // /
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(left_num / right_num);
        return;
      }

      case Token::MOD: {  // %
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(std::fmod(left_num, right_num));
        return;
      }

      case Token::ADD: {  // +
        // section 11.6.1 NOTE
        // no hint is provided in the calls to ToPrimitive
        const JSVal lprim = lhs.ToPrimitive(context_, JSObject::NONE, CHECK);
        const JSVal rprim = rhs.ToPrimitive(context_, JSObject::NONE, CHECK);
        if (lprim.IsString() || rprim.IsString()) {
          const JSString* const lstr = lprim.ToString(context_, CHECK);
          const JSString* const rstr = rprim.ToString(context_, CHECK);
          context_->ret().set_value(JSString::New(context_, *lstr + *rstr));
          return;
        }
        assert(lprim.IsNumber() && rprim.IsNumber());
        const double left_num = lprim.ToNumber(context_, CHECK);
        const double right_num = rprim.ToNumber(context_, CHECK);
        context_->ret().set_value(left_num + right_num);
        return;
      }

      case Token::SUB: {  // -
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(left_num - right_num);
        return;
      }

      case Token::SHL: {  // <<
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(
            static_cast<double>(core::Conv::DoubleToInt32(left_num)
                   << (core::Conv::DoubleToInt32(right_num) & 0x1f)));
        return;
      }

      case Token::SAR: {  // >>
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(
            static_cast<double>(core::Conv::DoubleToInt32(left_num)
                   >> (core::Conv::DoubleToInt32(right_num) & 0x1f)));
        return;
      }

      case Token::SHR: {  // >>>
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(
            static_cast<double>(core::Conv::DoubleToUInt32(left_num)
                   >> (core::Conv::DoubleToInt32(right_num) & 0x1f)));
        return;
      }

      case Token::LT: {  // <
        const CompareKind res = Compare(lhs, rhs, true, CHECK);
        context_->ret().set_value(res == CMP_TRUE);
        return;
      }

      case Token::GT: {  // >
        const CompareKind res = Compare(rhs, lhs, false, CHECK);
        context_->ret().set_value(res == CMP_TRUE);
        return;
      }

      case Token::LTE: {  // <=
        const CompareKind res = Compare(rhs, lhs, false, CHECK);
        context_->ret().set_value(res == CMP_FALSE);
        return;
      }

      case Token::GTE: {  // >=
        const CompareKind res = Compare(lhs, rhs, true, CHECK);
        context_->ret().set_value(res == CMP_FALSE);
        return;
      }

      case Token::INSTANCEOF: {  // instanceof
        if (!rhs.IsObject()) {
          context_->set_error(JSErrorCode::TypeError);
          return;
        }
        JSObject* const robj = rhs.object();
        if (!robj->IsCallable()) {
          context_->set_error(JSErrorCode::TypeError);
          return;
        }
        bool res = robj->AsCallable()->HasInstance(context_, lhs, CHECK);
        context_->ret().set_value(res);
        return;
      }

      case Token::IN: {  // in
        if (!rhs.IsObject()) {
          context_->set_error(JSErrorCode::TypeError);
          return;
        }
        const JSString* const name = lhs.ToString(context_, CHECK);
        context_->ret().set_value(
            rhs.object()->HasProperty(context_->Intern(*name)));
        return;
      }

      case Token::EQ: {  // ==
        const bool res = AbstractEqual(lhs, rhs, CHECK);
        context_->ret().set_value(res);
        return;
      }

      case Token::NE: {  // !=
        const bool res = AbstractEqual(lhs, rhs, CHECK);
        context_->ret().set_value(!res);
        return;
      }

      case Token::EQ_STRICT: {  // ===
        context_->ret().set_value(StrictEqual(lhs, rhs));
        return;
      }

      case Token::NE_STRICT: {  // !==
        context_->ret().set_value(!StrictEqual(lhs, rhs));
        return;
      }

      // bitwise op
      case Token::BIT_AND: {  // &
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(
            static_cast<double>(core::Conv::DoubleToInt32(left_num)
                   & (core::Conv::DoubleToInt32(right_num))));
        return;
      }

      case Token::BIT_XOR: {  // ^
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(
            static_cast<double>(core::Conv::DoubleToInt32(left_num)
                   ^ (core::Conv::DoubleToInt32(right_num))));
        return;
      }

      case Token::BIT_OR: {  // |
        const double left_num = lhs.ToNumber(context_, CHECK);
        const double right_num = rhs.ToNumber(context_, CHECK);
        context_->ret().set_value(
            static_cast<double>(core::Conv::DoubleToInt32(left_num)
                   | (core::Conv::DoubleToInt32(right_num))));
        return;
      }

      case Token::COMMA:  // ,
        context_->set_ret(rhs);
        return;

      default:
        return;
    }
  }
}


void Interpreter::Visit(core::ConditionalExpression* cond) {
  EVAL(cond->cond());
  const JSVal expr = GetValue(context_->ret(), CHECK);
  const bool condition = expr.ToBoolean(CHECK);
  if (condition) {
    EVAL(cond->left());
    context_->ret() = GetValue(context_->ret(), CHECK);
    return;
  } else {
    EVAL(cond->right());
    context_->ret() = GetValue(context_->ret(), CHECK);
    return;
  }
}


void Interpreter::Visit(core::UnaryOperation* unary) {
  using core::Token;
  switch (unary->op()) {
    case Token::DELETE: {
      EVAL(unary->expr());
      if (!context_->ret().IsReference()) {
        context_->ret().set_value(true);
        return;
      }
      const JSReference* const ref = context_->ret().reference();
      if (ref->IsUnresolvableReference()) {
        if (ref->IsStrictReference()) {
          context_->set_error(JSErrorCode::SyntaxError);
          return;
        } else {
          context_->ret().set_value(true);
          return;
        }
      }
      if (ref->IsPropertyReference()) {
        JSObject* const obj = ref->base()->ToObject(context_, CHECK);
        const bool result = obj->Delete(ref->GetReferencedName(),
                                        ref->IsStrictReference(), CHECK);
        context_->ret().set_value(result);
      } else {
        assert(ref->base()->IsEnvironment());
        if (ref->IsStrictReference()) {
          context_->set_error(JSErrorCode::SyntaxError);
          return;
        }
        context_->ret().set_value(
            ref->base()->environment()->DeleteBinding(
                ref->GetReferencedName()));
      }
      return;
    }

    case Token::VOID: {
      EVAL(unary->expr());
      GetValue(context_->ret(), CHECK);
      context_->ret().set_undefined();
      return;
    }

    case Token::TYPEOF: {
      EVAL(unary->expr());
      if (context_->ret().IsReference()) {
        if (context_->ret().reference()->base()->IsUndefined()) {
          context_->ret().set_value(
              JSString::NewAsciiString(context_, "undefined"));
          return;
        }
      }
      const JSVal expr = GetValue(context_->ret(), CHECK);
      context_->ret().set_value(expr.TypeOf(context_));
      return;
    }

    case Token::INC: {
      EVAL(unary->expr());
      const JSVal expr = context_->ret();
      if (expr.IsReference()) {
        JSReference* const ref = expr.reference();
        if (ref->IsStrictReference() &&
            ref->base()->IsEnvironment()) {
          const Symbol sym = ref->GetReferencedName();
          if (sym == context_->Intern(eval_string) ||
              sym == context_->Intern(arguments_string)) {
            context_->set_error(JSErrorCode::SyntaxError);
            return;
          }
        }
      }
      const JSVal value = GetValue(expr, CHECK);
      const double old_value = value.ToNumber(context_, CHECK);
      const JSVal new_value(old_value + 1);
      PutValue(expr, new_value, CHECK);
      context_->set_ret(new_value);
      return;
    }

    case Token::DEC: {
      EVAL(unary->expr());
      const JSVal expr = context_->ret();
      if (expr.IsReference()) {
        JSReference* const ref = expr.reference();
        if (ref->IsStrictReference() &&
            ref->base()->IsEnvironment()) {
          const Symbol sym = ref->GetReferencedName();
          if (sym == context_->Intern(eval_string) ||
              sym == context_->Intern(arguments_string)) {
            context_->set_error(JSErrorCode::SyntaxError);
            return;
          }
        }
      }
      const JSVal value = GetValue(expr, CHECK);
      const double old_value = value.ToNumber(context_, CHECK);
      const JSVal new_value(old_value - 1);
      PutValue(expr, new_value, CHECK);
      context_->set_ret(new_value);
      return;
    }

    case Token::ADD: {
      EVAL(unary->expr());
      const JSVal expr = GetValue(context_->ret(), CHECK);
      const double val = expr.ToNumber(context_, CHECK);
      context_->ret().set_value(val);
      return;
    }

    case Token::SUB: {
      EVAL(unary->expr());
      const JSVal expr = GetValue(context_->ret(), CHECK);
      const double old_value = expr.ToNumber(context_, CHECK);
      context_->ret().set_value(-old_value);
      return;
    }

    case Token::BIT_NOT: {
      EVAL(unary->expr());
      const JSVal expr = GetValue(context_->ret(), CHECK);
      const double value = expr.ToNumber(context_, CHECK);
      context_->ret().set_value(
          static_cast<double>(~core::Conv::DoubleToInt32(value)));
      return;
    }

    case Token::NOT: {
      EVAL(unary->expr());
      const JSVal expr = GetValue(context_->ret(), CHECK);
      const bool value = expr.ToBoolean(CHECK);
      context_->ret().set_value(!value);
      return;
    }

    default:
      UNREACHABLE();
  }
}


void Interpreter::Visit(core::PostfixExpression* postfix) {
  EVAL(postfix->expr());
  JSVal lref = context_->ret();
  if (lref.IsReference()) {
    const JSReference* const ref = lref.reference();
    if (ref->IsStrictReference() &&
        ref->base()->IsEnvironment()) {
      const Symbol sym = ref->GetReferencedName();
      if (sym == context_->Intern(eval_string) ||
          sym == context_->Intern(arguments_string)) {
        context_->set_error(JSErrorCode::SyntaxError);
        return;
      }
    }
  }
  JSVal old = GetValue(lref, CHECK);
  const double& value = old.ToNumber(context_, CHECK);
  const double new_value = value +
      ((postfix->op() == core::Token::INC) ? 1 : -1);
  PutValue(lref, JSVal(new_value), CHECK);
  context_->set_ret(old);
}


void Interpreter::Visit(core::StringLiteral* str) {
  context_->ret().set_value(JSString::New(context_, str->value()));
}


void Interpreter::Visit(core::NumberLiteral* num) {
  context_->ret().set_value(num->value());
}


void Interpreter::Visit(core::Identifier* ident) {
  // section 10.3.1 Identifier Resolution
  JSEnv* env = context_->lexical_env();
  context_->ret().set_value(
      GetIdentifierReference(env,
                             context_->Intern(ident->value()),
                             context_->IsStrict()));
}


void Interpreter::Visit(core::ThisLiteral* literal) {
  context_->ret().set_value(context_->this_binding());
}


void Interpreter::Visit(core::NullLiteral* lit) {
  context_->ret().set_null();
}


void Interpreter::Visit(core::TrueLiteral* lit) {
  context_->ret().set_value(true);
}


void Interpreter::Visit(core::FalseLiteral* lit) {
  context_->ret().set_value(false);
}


void Interpreter::Visit(core::Undefined* lit) {
  context_->ret().set_undefined();
}


void Interpreter::Visit(core::RegExpLiteral* regexp) {
  context_->ret().set_value(JSRegExp::New(regexp->value(), regexp->flags()));
}


void Interpreter::Visit(core::ArrayLiteral* literal) {
  // when in parse phase, have already removed last elision.
  JSArray* const ary = JSArray::New(context_);
  std::size_t current = 0;
  std::tr1::array<char, 30> buffer;
  BOOST_FOREACH(core::Expression* const expr, literal->items()) {
    if (expr) {
      EVAL(expr);
      const JSVal value = GetValue(context_->ret(), CHECK);
      std::snprintf(buffer.data(), buffer.size(),
                    Format<std::size_t>::printf, current);
      const Symbol index = context_->Intern(buffer.data());
      ary->DefineOwnProperty(
          context_, index,
          new DataDescriptor(value, PropertyDescriptor::WRITABLE |
                                    PropertyDescriptor::ENUMERABLE |
                                    PropertyDescriptor::CONFIGURABLE),
          false, CHECK);
    }
    ++current;
  }
  ary->Put(context_, context_->Intern("length"),
           JSVal(static_cast<double>(current)), false, CHECK);
  context_->ret().set_value(ary);
}


void Interpreter::Visit(core::ObjectLiteral* literal) {
  using std::tr1::get;
  using core::ObjectLiteral;
  JSObject* const obj = JSObject::New(context_);

  // section 11.1.5
  BOOST_FOREACH(const ObjectLiteral::Property& prop, literal->properties()) {
    const ObjectLiteral::PropertyDescriptorType type(get<0>(prop));
    const core::Identifier* const ident = get<1>(prop);
    const Symbol name = context_->Intern(ident->value());
    PropertyDescriptor* desc = NULL;
    if (type == ObjectLiteral::DATA) {
      EVAL(get<2>(prop));
      const JSVal value = GetValue(context_->ret(), CHECK);
      desc = new DataDescriptor(value,
                                PropertyDescriptor::WRITABLE |
                                PropertyDescriptor::ENUMERABLE |
                                PropertyDescriptor::CONFIGURABLE);
    } else {
      EVAL(get<2>(prop));
      if (type == ObjectLiteral::GET) {
        desc = new AccessorDescriptor(context_->ret().object(), NULL,
                                      PropertyDescriptor::ENUMERABLE |
                                      PropertyDescriptor::CONFIGURABLE);
      } else {
        desc = new AccessorDescriptor(NULL, context_->ret().object(),
                                      PropertyDescriptor::ENUMERABLE |
                                      PropertyDescriptor::CONFIGURABLE);
      }
    }
    // section 11.1.5 step 4
    // Syntax error detection is already passed in parser phase.
    // Because syntax error is early error (section 16 Errors)
    // syntax error is reported at parser phase.
    // So, in interpreter phase, there's nothing to do.
    obj->DefineOwnProperty(context_, name, desc, false, CHECK);
  }
  context_->ret().set_value(obj);
}


void Interpreter::Visit(core::FunctionLiteral* func) {
  context_->ret().set_value(
      JSCodeFunction::New(context_, func, context_->lexical_env()));
}


void Interpreter::Visit(core::IdentifierAccess* prop) {
  EVAL(prop->target());
  const JSVal base_value = GetValue(context_->ret(), CHECK);
  base_value.CheckObjectCoercible(CHECK);
  const Symbol sym = context_->Intern(prop->key()->value());
  context_->ret().set_value(
      JSReference::New(context_, base_value, sym, context_->IsStrict()));
}


void Interpreter::Visit(core::IndexAccess* prop) {
  EVAL(prop->target());
  const JSVal base_value = GetValue(context_->ret(), CHECK);
  EVAL(prop->key());
  const JSVal name_value = GetValue(context_->ret(), CHECK);
  base_value.CheckObjectCoercible(CHECK);
  const JSString* const name = name_value.ToString(context_, CHECK);
  context_->ret().set_value(
      JSReference::New(context_,
                       base_value,
                       context_->Intern(*name),
                       context_->IsStrict()));
}


void Interpreter::Visit(core::FunctionCall* call) {
  EVAL(call->target());
  const JSVal target = context_->ret();

  Arguments args(context_, call->args().size());
  std::size_t n = 0;
  BOOST_FOREACH(core::Expression* const expr, call->args()) {
    EVAL(expr);
    args[n++] = GetValue(context_->ret(), CHECK);
  }

  const JSVal func = GetValue(target, CHECK);
  if (!func.IsObject()) {
    context_->set_error(JSErrorCode::TypeError);
    return;
  }
  if (!func.object()->IsCallable()) {
    context_->set_error(JSErrorCode::TypeError);
    return;
  }
  if (target.IsReference()) {
    const JSReference* const ref = target.reference();
    if (ref->IsPropertyReference()) {
      args.set_this_binding(*(ref->base()));
    } else {
      assert(ref->base()->IsEnvironment());
      args.set_this_binding(ref->base()->environment()->ImplicitThisValue());
    }
  } else {
    args.set_this_binding(JSVal::Undefined());
  }

  context_->ret() = func.object()->AsCallable()->Call(args, CHECK);
}


void Interpreter::Visit(core::ConstructorCall* call) {
  EVAL(call->target());
  const JSVal target = context_->ret();

  Arguments args(context_, call->args().size());
  std::size_t n = 0;
  BOOST_FOREACH(core::Expression* const expr, call->args()) {
    EVAL(expr);
    args[n++] = GetValue(context_->ret(), CHECK);
  }

  const JSVal func = GetValue(target, CHECK);
  if (!func.IsObject()) {
    context_->set_error(JSErrorCode::TypeError);
    return;
  }
  if (!func.object()->IsCallable()) {
    context_->set_error(JSErrorCode::TypeError);
    return;
  }
  JSFunction* const constructor = func.object()->AsCallable();
  JSObject* const obj = JSObject::New(context_);
  const JSVal proto = constructor->Get(
      context_, context_->Intern("prototype"), CHECK);
  if (proto.IsObject()) {
    obj->set_prototype(proto.object());
  }
  args.set_this_binding(JSVal(obj));
  const JSVal result = constructor->Call(args, CHECK);
  if (result.IsObject()) {
    context_->ret() = result;
  } else {
    context_->ret().set_value(obj);
  }
}


// section 8.7.1 GetValue
JSVal Interpreter::GetValue(const JSVal& val, JSErrorCode::Type* error) {
  if (!val.IsReference()) {
    return val;
  }
  const JSReference* const ref = val.reference();
  const JSVal* const base = ref->base();
  if (ref->IsUnresolvableReference()) {
    *error = JSErrorCode::TypeError;
    return JSVal::Undefined();
  }
  if (ref->IsPropertyReference()) {
    if (ref->HasPrimitiveBase()) {
      // section 8.7.1 special [[Get]]
      const JSObject* const o = base->ToObject(context_, error);
      if (*error) {
        return JSVal::Undefined();
      }
      PropertyDescriptor* desc = o->GetProperty(ref->GetReferencedName());
      if (!desc) {
        return JSVal::Undefined();
      }
      if (desc->IsDataDescriptor()) {
        return desc->AsDataDescriptor()->value();
      } else {
        assert(desc->IsAccessorDescriptor());
        AccessorDescriptor* ac = desc->AsAccessorDescriptor();
        if (ac->get()) {
          JSVal res = ac->get()->AsCallable()->Call(Arguments(context_, *base),
                                                    error);
          if (*error) {
            return JSVal::Undefined();
          }
          return res;
        } else {
          return JSVal::Undefined();
        }
      }
    } else {
      JSVal res = base->object()->Get(context_,
                                      ref->GetReferencedName(), error);
      if (*error) {
        return JSVal::Undefined();
      }
      return res;
    }
    return JSVal::Undefined();
  } else {
    JSVal res = base->environment()->GetBindingValue(
        context_, ref->GetReferencedName(), ref->IsStrictReference(), error);
    if (*error) {
      return JSVal::Undefined();
    }
    return res;
  }
}


#define ERRCHECK  error);\
  if (*error) {\
    return;\
  }\
  ((void)0


// section 8.7.2 PutValue
void Interpreter::PutValue(const JSVal& val, const JSVal& w,
                           JSErrorCode::Type* error) {
  if (!val.IsReference()) {
    *error = JSErrorCode::ReferenceError;
    return;
  }
  const JSReference* const ref = val.reference();
  const JSVal* const base = ref->base();
  if (ref->IsUnresolvableReference()) {
    if (ref->IsStrictReference()) {
      *error = JSErrorCode::ReferenceError;
      return;
    }
    context_->global_obj()->Put(context_, ref->GetReferencedName(),
                                w, false, ERRCHECK);
  } else if (ref->IsPropertyReference()) {
    if (ref->HasPrimitiveBase()) {
      const Symbol sym = ref->GetReferencedName();
      const bool th = ref->IsStrictReference();
      JSObject* const o = base->ToObject(context_, ERRCHECK);
      if (!o->CanPut(sym)) {
        if (th) {
          *error = JSErrorCode::TypeError;
        }
        return;
      }
      PropertyDescriptor* const own_desc = o->GetOwnProperty(sym);
      if (own_desc && own_desc->IsDataDescriptor()) {
        if (th) {
          *error = JSErrorCode::TypeError;
        }
        return;
      }
      PropertyDescriptor* const desc = o->GetProperty(sym);
      if (desc && desc->IsAccessorDescriptor()) {
        AccessorDescriptor* ac = desc->AsAccessorDescriptor();
        assert(ac->set());
        ac->set()->AsCallable()->Call(Arguments(context_, *base), ERRCHECK);
      } else {
        if (th) {
          *error = JSErrorCode::TypeError;
        }
      }
      return;
    } else {
      base->object()->Put(context_, ref->GetReferencedName(), w,
                          ref->IsStrictReference(), ERRCHECK);
    }
  } else {
    assert(base->environment());
    base->environment()->SetMutableBinding(context_,
                                           ref->GetReferencedName(), w,
                                           ref->IsStrictReference(), ERRCHECK);
  }
}


#undef ERRCHECK


bool Interpreter::SameValue(const JSVal& lhs, const JSVal& rhs) {
  if (lhs.type() != rhs.type()) {
    return false;
  }
  if (lhs.IsUndefined()) {
    return true;
  }
  if (lhs.IsNull()) {
    return true;
  }
  if (lhs.IsNumber()) {
    // TODO(Constellation)
    // more exactly number comparison
    const double& lhsv = lhs.number();
    const double& rhsv = rhs.number();
    if (std::isnan(lhsv) && std::isnan(rhsv)) {
      return true;
    }
    if (lhsv == rhsv) {
      if (std::signbit(lhsv) && std::signbit(rhsv)) {
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
  if (lhs.IsString()) {
    return *(lhs.string()) == *(rhs.string());
  }
  if (lhs.IsBoolean()) {
    return lhs.boolean() == rhs.boolean();
  }
  if (lhs.IsObject()) {
    return lhs.object() == rhs.object();
  }
  return false;
}


bool Interpreter::StrictEqual(const JSVal& lhs, const JSVal& rhs) {
  if (lhs.type() != rhs.type()) {
    return false;
  }
  if (lhs.IsUndefined()) {
    return true;
  }
  if (lhs.IsNull()) {
    return true;
  }
  if (lhs.IsNumber()) {
    const double& lhsv = lhs.number();
    const double& rhsv = rhs.number();
    if (std::isnan(lhsv) || std::isnan(rhsv)) {
      return false;
    }
    return lhsv == rhsv;
  }
  if (lhs.IsString()) {
    return *(lhs.string()) == *(rhs.string());
  }
  if (lhs.IsBoolean()) {
    return lhs.boolean() == rhs.boolean();
  }
  if (lhs.IsObject()) {
    return lhs.object() == rhs.object();
  }
  return false;
}


#define ABSTRACT_CHECK\
  CHECK_TO_WITH(error, false)


bool Interpreter::AbstractEqual(const JSVal& lhs, const JSVal& rhs,
                                JSErrorCode::Type* error) {
  if (lhs.type() == rhs.type()) {
    if (lhs.IsUndefined()) {
      return true;
    }
    if (lhs.IsNull()) {
      return true;
    }
    if (lhs.IsNumber()) {
      const double& lhsv = lhs.number();
      const double& rhsv = rhs.number();
      if (std::isnan(lhsv) || std::isnan(rhsv)) {
        return false;
      }
      return lhsv == rhsv;
    }
    if (lhs.IsString()) {
      return *(lhs.string()) == *(rhs.string());
    }
    if (lhs.IsBoolean()) {
      return lhs.boolean() == rhs.boolean();
    }
    if (lhs.IsObject()) {
      return lhs.object() == rhs.object();
    }
    return false;
  }
  if (lhs.IsNull() && rhs.IsUndefined()) {
    return true;
  }
  if (lhs.IsUndefined() && rhs.IsNull()) {
    return true;
  }
  if (lhs.IsNumber() && rhs.IsString()) {
    const double num = rhs.ToNumber(context_, ABSTRACT_CHECK);
    return AbstractEqual(lhs, JSVal(num), error);
  }
  if (lhs.IsString() && rhs.IsNumber()) {
    const double num = lhs.ToNumber(context_, ABSTRACT_CHECK);
    return AbstractEqual(JSVal(num), rhs, error);
  }
  if (lhs.IsBoolean()) {
    const double num = lhs.ToNumber(context_, ABSTRACT_CHECK);
    return AbstractEqual(JSVal(num), rhs, error);
  }
  if (rhs.IsBoolean()) {
    const double num = rhs.ToNumber(context_, ABSTRACT_CHECK);
    return AbstractEqual(lhs, JSVal(num), error);
  }
  if ((lhs.IsString() || lhs.IsNumber()) &&
      rhs.IsObject()) {
    const JSVal prim = rhs.ToPrimitive(context_,
                                       JSObject::NONE, ABSTRACT_CHECK);
    return AbstractEqual(lhs, prim, error);
  }
  if (lhs.IsObject() &&
      (rhs.IsString() || rhs.IsNumber())) {
    const JSVal prim = lhs.ToPrimitive(context_,
                                       JSObject::NONE, ABSTRACT_CHECK);
    return AbstractEqual(prim, rhs, error);
  }
  return false;
}
#undef ABSTRACT_CHECK


// section 11.8.5
#define LT_CHECK\
  CHECK_TO_WITH(error, CMP_ERROR)


Interpreter::CompareKind Interpreter::Compare(const JSVal& lhs,
                                              const JSVal& rhs,
                                              bool left_first,
                                              JSErrorCode::Type* error) {
  JSVal px;
  JSVal py;
  if (left_first) {
    px = lhs.ToPrimitive(context_, JSObject::NUMBER, LT_CHECK);
    py = rhs.ToPrimitive(context_, JSObject::NUMBER, LT_CHECK);
  } else {
    py = rhs.ToPrimitive(context_, JSObject::NUMBER, LT_CHECK);
    px = lhs.ToPrimitive(context_, JSObject::NUMBER, LT_CHECK);
  }
  if (px.IsString() && py.IsString()) {
    // step 4
    return (*(px.string()) < *(py.string())) ? CMP_TRUE : CMP_FALSE;
  } else {
    const double nx = px.ToNumber(context_, LT_CHECK);
    const double ny = py.ToNumber(context_, LT_CHECK);
    if (std::isnan(nx) || std::isnan(ny)) {
      return CMP_UNDEFINED;
    }
    if (nx == ny) {
      if (std::signbit(nx) != std::signbit(ny)) {
        return CMP_FALSE;
      }
      return CMP_FALSE;
    }
    if (nx == std::numeric_limits<double>::infinity()) {
      return CMP_FALSE;
    }
    if (ny == std::numeric_limits<double>::infinity()) {
      return CMP_TRUE;
    }
    if (ny == (-std::numeric_limits<double>::infinity())) {
      return CMP_FALSE;
    }
    if (nx == (-std::numeric_limits<double>::infinity())) {
      return CMP_TRUE;
    }
    return (nx < ny) ? CMP_TRUE : CMP_FALSE;
  }
}
#undef LT_CHECK


JSDeclEnv* Interpreter::NewDeclarativeEnvironment(Context* ctx, JSEnv* env) {
  return JSDeclEnv::New(ctx, env);
}


JSObjectEnv* Interpreter::NewObjectEnvironment(Context* ctx,
                                               JSObject* val, JSEnv* env) {
  assert(val);
  return JSObjectEnv::New(ctx, env, val);
}


JSReference* Interpreter::GetIdentifierReference(JSEnv* lex,
                                                 Symbol name, bool strict) {
  JSEnv* env = lex;
  while (env) {
    if (env->HasBinding(name)) {
      return JSReference::New(context_, JSVal(env), name, strict);
    } else {
      env = env->outer();
    }
  }
  return JSReference::New(context_, JSVal::Undefined(), name, strict);
}

#undef CHECK
#undef ERR_CHECK
#undef RETURN_STMT
#undef ABRUPT

} }  // namespace iv::lv5
