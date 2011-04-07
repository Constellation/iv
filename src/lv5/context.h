#ifndef _IV_LV5_CONTEXT_H_
#define _IV_LV5_CONTEXT_H_
#include <tr1/unordered_map>
#include <tr1/type_traits>
#include "stringpiece.h"
#include "ustringpiece.h"
#include "noncopyable.h"
#include "enable_if.h"
#include "ast.h"
#include "lv5/jsval.h"
#include "lv5/jsenv.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
#include "lv5/global_data.h"
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
  friend Symbol context::Intern(Context* ctx, uint32_t index);
  friend Symbol context::Intern(Context* ctx, double number);

  friend void RegisterLiteralRegExp(Context* ctx, JSRegExpImpl* reg);

  enum Mode {
    NORMAL,
    BREAK,
    CONTINUE,
    RETURN,
    THROW
  };

  Context();

  const JSObject* global_obj() const {
    return global_data_.global_obj();
  }

  JSObject* global_obj() {
    return global_data_.global_obj();
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
    const Symbol name = context::Intern(this, func_name);
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
    const Symbol name = context::Intern(this, func_name);
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
    return obj.class_name() == global_data_.Array_symbol();
  }

  double Random();

  GlobalData* global_data() {
    return &global_data_;
  }

  const GlobalData* global_data() const {
    return &global_data_;
  }

  JSString* ToString(Symbol sym);

  bool InCurrentLabelSet(const AnonymousBreakableStatement* stmt) const;
  bool InCurrentLabelSet(const NamedOnlyBreakableStatement* stmt) const;

  const Class& Cls(Symbol name);
  const Class& Cls(const core::StringPiece& str);

  VMStack* stack() {
    return stack_resource_.stack();
  }

 private:
  GlobalData global_data_;
  StackResource stack_resource_;
  JSEnv* lexical_env_;
  JSEnv* variable_env_;
  JSEnv* global_env_;
  JSVal binding_;
  Interpreter interp_;
  Mode mode_;
  JSVal ret_;
  const BreakableStatement* target_;
  Error error_;
  trace::HashMap<Symbol, Class>::type builtins_;
  bool strict_;
  std::size_t generate_script_counter_;
  JSScript* current_script_;
  JSInlinedFunction<&runtime::ThrowTypeError, 0> throw_type_error_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_CONTEXT_H_
