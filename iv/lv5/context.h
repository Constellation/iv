#ifndef IV_LV5_CONTEXT_H_
#define IV_LV5_CONTEXT_H_
#include <cstddef>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#include <iv/noncopyable.h>
#include <iv/space.h>
#include <iv/aero/aero.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/class.h>
#include <iv/lv5/error.h>
#include <iv/lv5/global_data.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/stack.h>

namespace iv {
namespace lv5 {
namespace runtime {
JSVal ThrowTypeError(const Arguments& args, Error* e);
}  // namespace runtime
namespace radio {
class Core;
}  // namespace radio
namespace bind {
class Object;
}  // namespace bind

class JSObjectEnv;

class Context : public radio::HeapObject<radio::POINTER_CLEANUP> {
 public:
  friend Symbol context::Intern(Context* ctx, const core::StringPiece& str);
  friend Symbol context::Intern(Context* ctx, const core::UStringPiece& str);
  friend Symbol context::Intern(Context* ctx, uint32_t index);
  friend Symbol context::Intern(Context* ctx, double number);
  friend Symbol context::Intern64(Context* ctx, uint64_t index);

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

  inline JSVal* StackGain(std::size_t size) { return stack_->Gain(size); }

  inline void StackRestore(JSVal* ptr) { stack_->Restore(ptr); }

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
        JSInlinedFunction<FunctionConstructor, 1>::NewPlain(
            this, context::Intern(this, "Function")),
        JSInlinedFunction<GlobalEval, 1>::NewPlain(this, symbol::eval()));
  }

  JSFunction* throw_type_error() {
    return throw_type_error_;
  }

  double Random() { return global_data_.Random(); }

  GlobalData* global_data() { return &global_data_; }

  const GlobalData* global_data() const { return &global_data_; }

  core::Space* regexp_allocator() { return &regexp_allocator_; }

  aero::VM* regexp_vm() { return &regexp_vm_; }

  core::SymbolTable* symbol_table() { return global_data_.symbol_table(); }

  void MarkChildren(radio::Core* core) { }

  void RegisterStack(Stack* stack) { stack_ = stack; }

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

  // ES.next
  void InitMap(const ClassSlot& func_cls,
               JSObject* obj_proto, bind::Object* global_binder);

  void InitSet(const ClassSlot& func_cls,
               JSObject* obj_proto, bind::Object* global_binder);

  GlobalData global_data_;
  JSInlinedFunction<&runtime::ThrowTypeError, 0>* throw_type_error_;
  JSObjectEnv* global_env_;
  core::Space regexp_allocator_;  // for RegExp AST
  aero::VM regexp_vm_;
  Stack* stack_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_CONTEXT_H_
