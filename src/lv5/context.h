#ifndef _IV_LV5_CONTEXT_H_
#define _IV_LV5_CONTEXT_H_
#include <tr1/unordered_map>
#include "stringpiece.h"
#include "ustringpiece.h"
#include "noncopyable.h"
#include "ast.h"
#include "jsenv.h"
#include "jsval.h"
#include "jsobject.h"
#include "jsfunction.h"
#include "jserrorcode.h"
#include "symboltable.h"
#include "class.h"

namespace iv {
namespace lv5 {
class Interpreter;
class Context : private core::Noncopyable<Context>::type {
 public:

  enum Mode {
    NORMAL,
    BREAK,
    CONTINUE,
    RETURN,
    THROW
  };
  explicit Context(Interpreter* interp);
  const JSObject* global_obj() const {
    return &global_obj_;
  }
  JSObject* global_obj() {
    return &global_obj_;
  }
  JSEnv* lexical_env() const {
    return lexical_env_;
  }
  void set_lexical_env(JSEnv* env) {
    lexical_env_ = env;
  }
  JSEnv* variable_env() const {
    return variable_env_;
  }
  void set_variable_env(JSEnv* env) {
    variable_env_ = env;
  }
  JSObject* this_binding() const {
    return binding_;
  }
  JSObject* This() const {
    return binding_;
  }
  void set_this_binding(JSObject* binding) {
    binding_ = binding;
  }
  Interpreter* interp() const {
    return interp_;
  }
  Mode mode() const {
    return mode_;
  }
  template<Mode m>
  bool IsMode() const {
    return mode_ == m;
  }
  void set_mode(Mode mode) {
    mode_ = mode;
  }
  bool IsStrict() const {
    return strict_;
  }
  void set_strict(bool strict) {
    strict_ = strict;
  }
  const JSErrorCode::Type* error() const {
    return &error_;
  }
  JSErrorCode::Type* error() {
    return &error_;
  }
  void set_error(JSErrorCode::Type error) {
    error_ = error;
  }
  const JSVal& ret() const {
    return ret_;
  }
  JSVal& ret() {
    return ret_;
  }
  void set_ret(const JSVal& ret) {
    ret_ = ret;
  }
  void SetStatement(Mode mode, const JSVal& val,
                    core::BreakableStatement* target) {
    mode_ = mode;
    ret_ = val;
    target_ = target;
  }
  bool IsError() const {
    return error_;
  }
  const core::BreakableStatement* target() const {
    return target_;
  }
  core::BreakableStatement* target() {
    return target_;
  }

  template<typename Func>
  void DefineFunction(const Func& f, core::StringPiece func_name) {
    JSFunction* const func = JSNativeFunction::New(this, f);
    const Symbol name = Intern(func_name);
    variable_env_->CreateMutableBinding(this, name, false);
    variable_env_->SetMutableBinding(this,
                                     name,
                                     JSVal(func), strict_, &error_);
  }

  void Prelude();

  const Class& Cls(Symbol name);
  const Class& Cls(core::StringPiece str);

  Symbol Intern(core::StringPiece str);
  Symbol Intern(core::UStringPiece str);
  JSString* ToString(Symbol sym);
  bool InCurrentLabelSet(const core::AnonymousBreakableStatement* stmt) const;
  bool InCurrentLabelSet(const core::NamedOnlyBreakableStatement* stmt) const;
 private:
  JSObject global_obj_;
  JSEnv* lexical_env_;
  JSEnv* variable_env_;
  JSObject* binding_;
  SymbolTable table_;
  Interpreter* interp_;
  Mode mode_;
  JSVal ret_;
  core::BreakableStatement* target_;
  JSErrorCode::Type error_;
  std::tr1::unordered_map<Symbol, Class> builtins_;
  bool strict_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_CONTEXT_H_
