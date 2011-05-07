#ifndef _IV_LV5_CONTEXT_H_
#define _IV_LV5_CONTEXT_H_
#include <cstddef>
#include "stringpiece.h"
#include "ustringpiece.h"
#include "noncopyable.h"
#include "lv5.h"
#include "lv5/jsval.h"
#include "lv5/jsenv.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
#include "lv5/class.h"
#include "lv5/error.h"
#include "lv5/jsast.h"
#include "lv5/global_data.h"
#include "lv5/context_utils.h"
#include "lv5/stack_resource.h"
#include "lv5/vm_stack.h"

namespace iv {
namespace lv5 {
namespace runtime {

JSVal ThrowTypeError(const Arguments& args, Error* error);

}  // namespace iv::lv5::runtime

class SymbolChecker;
class JSEnv;

class Context : private core::Noncopyable<> {
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

  template<typename Func>
  void DefineFunction(const Func& f,
                      const core::StringPiece& func_name,
                      std::size_t n) {
    Error error;
    JSFunction* const func = JSNativeFunction::New(this, f, n);
    const Symbol name = context::Intern(this, func_name);
    variable_env_->CreateMutableBinding(this, name, false, ERROR_VOID(&error));
    variable_env_->SetMutableBinding(this,
                                     name,
                                     func, false, &error);
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  void DefineFunction(const core::StringPiece& func_name) {
    Error error;
    JSFunction* const f = JSInlinedFunction<func, n>::New(this);
    const Symbol name = context::Intern(this, func_name);
    variable_env_->CreateMutableBinding(this, name, false, ERROR_VOID(&error));
    variable_env_->SetMutableBinding(this, name,
                                     f, false, &error);
  }

  void Initialize();

  JSFunction* throw_type_error() {
    return &throw_type_error_;
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

  VMStack* stack() {
    return stack_resource_.stack();
  }

 private:
  GlobalData global_data_;
  JSInlinedFunction<&runtime::ThrowTypeError, 0> throw_type_error_;
  StackResource stack_resource_;
  JSEnv* lexical_env_;
  JSEnv* variable_env_;
  JSEnv* global_env_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_CONTEXT_H_
