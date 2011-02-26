#ifndef _IV_LV5_CONTEXT_H_
#define _IV_LV5_CONTEXT_H_
#include <tr1/unordered_map>
#include <tr1/random>
#include <tr1/type_traits>
#include "xorshift.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "noncopyable.h"
#include "enable_if.h"
#include "ast.h"
#include "lv5/jsval.h"
#include "lv5/jsenv.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
#include "lv5/symboltable.h"
#include "lv5/class.h"
#include "lv5/interpreter.h"
#include "lv5/error.h"
#include "lv5/jsast.h"
#include "lv5/gc_template.h"
#include "lv5/context_utils.h"
#include "lv5/stack_resource.h"
#include "lv5/vm_stack.h"

namespace iv {
namespace lv5 {
namespace runtime {
JSVal ThrowTypeError(const Arguments& args, Error* error);
}  // namespace iv::lv5::runtime

class SymbolChecker;
class JSScript;
class JSEnv;

class Context : private core::Noncopyable<Context>::type {
 public:
  friend class SymbolChecker;
  friend const core::UString& context::GetSymbolString(const Context* ctx,
                                                       const Symbol& sym);
  friend const Class& context::Cls(Context* ctx, const Symbol& name);
  friend const Class& context::Cls(Context* ctx,
                                   const core::StringPiece& str);
  friend Symbol context::Intern(Context* ctx, const core::StringPiece& str);
  friend Symbol context::Intern(Context* ctx, const core::UStringPiece& str);
  friend Symbol context::Intern(Context* ctx, const Identifier& ident);
  friend Symbol context::Intern(Context* ctx, uint32_t index);
  friend Symbol context::Intern(Context* ctx, double number);

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
  JSEnv* global_env() const {
    return global_env_;
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

  void Return(JSVal val) {
    ret_ = val;
  }

  void SetStatement(Mode mode, const JSVal& val,
                    const BreakableStatement* target) {
    mode_ = mode;
    ret_ = val;
    target_ = target;
  }
  bool IsError() const {
    return error_;
  }
  const BreakableStatement* target() const {
    return target_;
  }

  template<typename Func>
  void DefineFunction(const Func& f,
                      const core::StringPiece& func_name,
                      std::size_t n) {
    JSFunction* const func = JSNativeFunction::New(this, f, n);
    const Symbol name = Intern(func_name);
    variable_env_->CreateMutableBinding(this, name, false, &error_);
    if (error_) {
      return;
    }
    variable_env_->SetMutableBinding(this,
                                     name,
                                     func, strict_, &error_);
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  void DefineFunction(const core::StringPiece& func_name) {
    JSFunction* const f = JSInlinedFunction<func, n>::New(this);
    const Symbol name = Intern(func_name);
    variable_env_->CreateMutableBinding(this, name, false, &error_);
    if (error_) {
      return;
    }
    variable_env_->SetMutableBinding(this,
                                     name,
                                     f, strict_, &error_);
  }

  void Initialize();
  bool Run(JSScript* script);

  JSVal ErrorVal();

  Symbol Intern(const core::StringPiece& str);
  Symbol Intern(const core::UStringPiece& str);
  Symbol Intern(const Identifier& ident);
  Symbol InternIndex(uint32_t index);
  Symbol InternDouble(double number);

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
    return prototype_symbol_;
  }
  inline Symbol constructor_symbol() const {
    return constructor_symbol_;
  }
  inline Symbol Array_symbol() const {
    return Array_symbol_;
  }
  JSFunction* throw_type_error() {
    return &throw_type_error_;
  }
  JSScript* current_script() const {
    return current_script_;
  }
  void set_current_script(JSScript* script) {
    current_script_ = script;
  }
  bool IsShouldGC() {
    ++generate_script_counter_;
    if (generate_script_counter_ > 30) {
      generate_script_counter_ = 0;
      return true;
    } else {
      return false;
    }
  }
  bool IsArray(const JSObject& obj) {
    return obj.class_name() == Array_symbol_;
  }
  double Random();
  JSString* ToString(Symbol sym);
  bool InCurrentLabelSet(const AnonymousBreakableStatement* stmt) const;
  bool InCurrentLabelSet(const NamedOnlyBreakableStatement* stmt) const;

  const Class& Cls(Symbol name);
  const Class& Cls(const core::StringPiece& str);

  VMStack* stack() {
    return stack_resource_.stack();
  }

 private:

  Symbol CheckIntern(const core::StringPiece& str, bool* found);
  Symbol CheckIntern(const core::UStringPiece& str, bool* found);
  Symbol CheckIntern(uint32_t index, bool* found);
  Symbol CheckIntern(double number, bool* found);

  const core::UString& GetSymbolString(Symbol sym) const;

  StackResource stack_resource_;
  JSObject global_obj_;
  JSEnv* lexical_env_;
  JSEnv* variable_env_;
  JSEnv* global_env_;
  JSVal binding_;
  SymbolTable table_;
  Interpreter interp_;
  Mode mode_;
  JSVal ret_;
  const BreakableStatement* target_;
  Error error_;
  trace::HashMap<Symbol, Class>::type builtins_;
  bool strict_;
  std::size_t generate_script_counter_;
  random_generator random_engine_;
  Symbol length_symbol_;
  Symbol eval_symbol_;
  Symbol arguments_symbol_;
  Symbol caller_symbol_;
  Symbol callee_symbol_;
  Symbol toString_symbol_;
  Symbol valueOf_symbol_;
  Symbol prototype_symbol_;
  Symbol constructor_symbol_;
  Symbol Array_symbol_;
  JSScript* current_script_;
  JSInlinedFunction<&runtime::ThrowTypeError, 0> throw_type_error_;
};

class SymbolChecker : private core::Noncopyable<SymbolChecker>::type {
 public:
  SymbolChecker(Context* ctx, const core::StringPiece& str)
    : found_(false),
      sym_(ctx->CheckIntern(str, &found_)) {
  }

  SymbolChecker(Context* ctx, const core::UStringPiece& str)
    : found_(false),
      sym_(ctx->CheckIntern(str, &found_)) {
  }

  SymbolChecker(Context* ctx, uint32_t index)
    : found_(false),
      sym_(ctx->CheckIntern(index, &found_)) {
  }

  SymbolChecker(Context* ctx, double number)
    : found_(false),
      sym_(ctx->CheckIntern(number, &found_)) {
  }

  bool Found() const {
    return found_;
  }

  Symbol symbol() const {
    return sym_;
  }

 private:
  bool found_;
  Symbol sym_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_CONTEXT_H_
