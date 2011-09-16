#ifndef IV_LV5_CONTEXT_H_
#define IV_LV5_CONTEXT_H_
#include <cstddef>
#include "stringpiece.h"
#include "ustringpiece.h"
#include "noncopyable.h"
#include "lv5/error_check.h"
#include "lv5/jsval.h"
#include "lv5/jsenv.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
#include "lv5/class.h"
#include "lv5/error.h"
#include "lv5/specialized_ast.h"
#include "lv5/global_data.h"
#include "lv5/context_utils.h"

namespace iv {
namespace lv5 {
namespace runtime {

JSVal ThrowTypeError(const Arguments& args, Error* e);

}  // namespace runtime
namespace bind {
class Object;
}  // namespace bind

class JSObjectEnv;

class Context
  : public gc_cleanup,
    private core::Noncopyable<Context> {
 public:
  friend Symbol context::Intern(Context* ctx, const core::StringPiece& str);
  friend Symbol context::Intern(Context* ctx, const core::UStringPiece& str);
  friend Symbol context::Intern(Context* ctx, uint32_t index);
  friend Symbol context::Intern(Context* ctx, double number);

  friend void RegisterLiteralRegExp(Context* ctx, JSRegExpImpl* reg);

  Context();
  virtual ~Context() { }

  const JSGlobal* global_obj() const {
    return global_data_.global_obj();
  }

  JSGlobal* global_obj() {
    return global_data_.global_obj();
  }

  JSObjectEnv* global_env() const {
    return global_env_;
  }

  virtual JSVal* StackGain(std::size_t size) { return NULL; }
  virtual void StackRelease(std::size_t size) { }

  template<typename Func>
  void DefineFunction(const Func& f,
                      const core::StringPiece& func_name,
                      std::size_t n) {
    Error e;
    JSFunction* const func = JSNativeFunction::New(this, f, n);
    const Symbol name = context::Intern(this, func_name);
    global_env_->CreateMutableBinding(this, name, false, IV_LV5_ERROR_VOID(&e));
    global_env_->SetMutableBinding(this, name, func, false, &e);
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  void DefineFunction(const core::StringPiece& func_name) {
    Error e;
    const Symbol name = context::Intern(this, func_name);
    JSFunction* const f = JSInlinedFunction<func, n>::New(this, name);
    global_env_->CreateMutableBinding(this, name, false, IV_LV5_ERROR_VOID(&e));
    global_env_->SetMutableBinding(this, name, f, false, &e);
  }

  template<JSAPI FunctionConstructor, JSAPI GlobalEval>
  void Initialize() {
    InitContext(
        JSInlinedFunction<FunctionConstructor, 1>::NewPlain(this),
        JSInlinedFunction<GlobalEval, 1>::NewPlain(this));
  }

  JSFunction* throw_type_error() {
    return &throw_type_error_;
  }

  double Random();

  GlobalData* global_data() {
    return &global_data_;
  }

  const GlobalData* global_data() const {
    return &global_data_;
  }

 private:
  void InitContext(JSFunction* func_constructor, JSFunction* eval_function);

  void InitGlobal(const ClassSlot& func_cls,
                  JSObject* obj_proto, JSFunction* eval_function,
                  bind::Object* global_binder);

  void InitArray(const ClassSlot& func_cls,
                 JSObject* obj_proto, bind::Object* global_binder);

  void InitString(const ClassSlot& func_cls,
                 JSObject* obj_proto, bind::Object* global_binder);

  void InitBoolean(const ClassSlot& func_cls,
                   JSObject* obj_proto, bind::Object* global_binder);

  void InitNumber(const ClassSlot& func_cls,
                  JSObject* obj_proto, bind::Object* global_binder);

  void InitMath(const ClassSlot& func_cls,
                JSObject* obj_proto, bind::Object* global_binder);

  void InitDate(const ClassSlot& func_cls,
                JSObject* obj_proto, bind::Object* global_binder);

  void InitRegExp(const ClassSlot& func_cls,
                 JSObject* obj_proto, bind::Object* global_binder);

  void InitError(const ClassSlot& func_cls,
                 JSObject* obj_proto, bind::Object* global_binder);

  void InitJSON(const ClassSlot& func_cls,
                JSObject* obj_proto, bind::Object* global_binder);

  GlobalData global_data_;
  JSInlinedFunction<&runtime::ThrowTypeError, 0> throw_type_error_;
  JSObjectEnv* global_env_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_CONTEXT_H_
