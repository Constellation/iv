#ifndef _IV_LV5_CONTEXT_H_
#define _IV_LV5_CONTEXT_H_
#include <tr1/unordered_map>
#include <tr1/random>
#include "xorshift.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "noncopyable.h"
#include "ast.h"
#include "jsenv.h"
#include "jsval.h"
#include "jsobject.h"
#include "jsfunction.h"
#include "symboltable.h"
#include "class.h"
#include "interpreter.h"
#include "error.h"

namespace iv {
namespace lv5 {
class Context : private core::Noncopyable<Context>::type {
 public:
  typedef iv::core::Xor128 random_engine_type;
  typedef std::tr1::uniform_real<double> random_distribution_type;
  typedef std::tr1::variate_generator<
      random_engine_type, random_distribution_type> random_generator;
  enum Mode {
    NORMAL,
    BREAK,
    CONTINUE,
    RETURN,
    THROW
  };
  Context();
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
  JSVal this_binding() const {
    return binding_;
  }
  JSVal This() const {
    return binding_;
  }
  void set_this_binding(const JSVal& binding) {
    binding_ = binding;
  }
  Interpreter* interp() {
    return &interp_;
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
  const Error& error() const {
    return error_;
  }
  Error* error() {
    return &error_;
  }
  void set_error(const Error& error) {
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

  template<class T>
  void Return(T val) {
    ret_.set_value(val);
  }
  void Return(const JSVal& val) {
    ret_ = val;
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
  void DefineFunction(const Func& f, const core::StringPiece& func_name) {
    JSFunction* const func = JSNativeFunction::New(this, f);
    const Symbol name = Intern(func_name);
    variable_env_->CreateMutableBinding(this, name, false);
    variable_env_->SetMutableBinding(this,
                                     name,
                                     func, strict_, &error_);
  }

  void Initialize();
  void Run(core::FunctionLiteral* global);

  const Class& Cls(Symbol name);
  const Class& Cls(const core::StringPiece& str);

  Symbol Intern(const core::StringPiece& str);
  Symbol Intern(const core::UStringPiece& str);
  inline Symbol length_symbol() const {
    return length_symbol_;
  }
  inline Symbol eval_symbol() const {
    return eval_symbol_;
  }
  inline Symbol arguments_symbol() const {
    return arguments_symbol_;
  }
  inline Symbol caller_symbol() const {
    return caller_symbol_;
  }
  inline Symbol callee_symbol() const {
    return callee_symbol_;
  }
  inline Symbol toString_symbol() const {
    return toString_symbol_;
  }
  inline Symbol valueOf_symbol() const {
    return valueOf_symbol_;
  }
  inline Symbol prototype_symbol() const {
    return valueOf_symbol_;
  }
  double Random();
  JSString* ToString(Symbol sym);
  bool InCurrentLabelSet(const core::AnonymousBreakableStatement* stmt) const;
  bool InCurrentLabelSet(const core::NamedOnlyBreakableStatement* stmt) const;
 private:
  JSObject global_obj_;
  JSEnv* lexical_env_;
  JSEnv* variable_env_;
  JSVal binding_;
  SymbolTable table_;
  Interpreter interp_;
  Mode mode_;
  JSVal ret_;
  core::BreakableStatement* target_;
  Error error_;
  std::tr1::unordered_map<Symbol, Class> builtins_;
  bool strict_;
  random_generator random_engine_;
  Symbol length_symbol_;
  Symbol eval_symbol_;
  Symbol arguments_symbol_;
  Symbol caller_symbol_;
  Symbol callee_symbol_;
  Symbol toString_symbol_;
  Symbol valueOf_symbol_;
  Symbol prototype_symbol_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_CONTEXT_H_
