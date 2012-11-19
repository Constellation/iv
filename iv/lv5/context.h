#ifndef IV_LV5_CONTEXT_H_
#define IV_LV5_CONTEXT_H_
#include <cstddef>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#include <iv/noncopyable.h>
#include <iv/space.h>
#include <iv/i18n.h>
#include <iv/aero/aero.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction_fwd.h>
#include <iv/lv5/class.h>
#include <iv/lv5/error.h>
#include <iv/lv5/global_data.h>
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
  friend void RegisterLiteralRegExp(Context* ctx, JSRegExpImpl* reg);

  Context(JSAPI fc, JSAPI ge);
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

  void Initialize();

  void Import(const core::StringPiece& name, JSObject* obj) {
    Error::Dummy e;
    const Symbol sym = Intern(name);
    global_env_->CreateMutableBinding(this, sym, false, IV_LV5_ERROR_VOID(&e));
    global_env_->SetMutableBinding(this, sym, obj, false, &e);
  }

  template<typename Func>
  void DefineFunction(const Func& f,
                      const core::StringPiece& func_name,
                      std::size_t n) {
    JSFunction* const func = JSNativeFunction::New(this, f, n);
    Import(func_name, func);
  }

  template<JSVal (*func)(const Arguments&, Error*), std::size_t n>
  void DefineFunction(const core::StringPiece& func_name) {
    const Symbol name = Intern(func_name);
    JSFunction* const f = JSInlinedFunction<func, n>::New(this, name);
    Import(func_name, f);
  }

  JSFunction* throw_type_error() {
    return throw_type_error_;
  }

  Symbol Intern(const core::StringPiece& str);
  Symbol Intern(const core::UStringPiece& str);
  Symbol Intern(const JSString* str);
  Symbol Intern(uint32_t index);
  Symbol Intern(double number);
  Symbol Intern64(uint64_t index);

  double Random() { return global_data_.Random(); }

  GlobalData* global_data() { return &global_data_; }

  const GlobalData* global_data() const { return &global_data_; }

  core::Space* regexp_allocator() { return &regexp_allocator_; }

  aero::VM* regexp_vm() { return &regexp_vm_; }

  core::SymbolTable* symbol_table() { return global_data_.symbol_table(); }

  void MarkChildren(radio::Core* core) { }

  void RegisterStack(Stack* stack) { stack_ = stack; }

  Stack* stack() const { return stack_; }

  core::i18n::I18N* i18n() { return &i18n_; }

  JSObject* intl() const { return intl_; }
  JSObject* number_format_prototype() const { return number_format_prototype_; }
  JSFunction* number_format_constructor() const { return number_format_constructor_; }
 private:
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

  void InitWeakMap(const ClassSlot& func_cls,
                   JSObject* obj_proto, bind::Object* global_binder);

  void InitSet(const ClassSlot& func_cls,
               JSObject* obj_proto, bind::Object* global_binder);

  void InitIntl(const ClassSlot& func_cls,
                JSObject* obj_proto, bind::Object* global_binder);

  void InitBinaryBlocks(const ClassSlot& func_cls,
                        JSObject* obj_proto, bind::Object* global_binder);

  template<typename TypedArray, Class::JSClassType CLS>
  void InitTypedArray(const ClassSlot& func_cls, bind::Object* global_binder);

  GlobalData global_data_;
  JSInlinedFunction<&runtime::ThrowTypeError, 0>* throw_type_error_;
  JSObjectEnv* global_env_;
  core::Space regexp_allocator_;  // for RegExp AST
  aero::VM regexp_vm_;
  Stack* stack_;
  JSAPI function_constructor_;
  JSAPI global_eval_;
  core::i18n::I18N i18n_;

  JSObject* intl_;
  JSObject* collator_constructor_;
  JSFunction* number_format_constructor_;
  JSObject* number_format_prototype_;
  JSObject* date_format_constructor_;
};

// ScopedArguments implementations
inline ScopedArguments::ScopedArguments(Context* ctx, std::size_t n, Error* e)
  : Arguments(ctx, n, e) {
  if (pointer ptr = ctx->stack()->Gain(size() + 1)) {
    std::fill<JSVal*, JSVal>(ptr, ptr + size() + 1, JSUndefined);
    stack_ = ptr + size();  // [this] position
  } else {
    // stack overflow
    e->Report(Error::Range, "maximum call stack size exceeded");
  }
}

inline ScopedArguments::~ScopedArguments() {
  if (stack_) {
    ctx()->stack()->Restore(stack_ - size());
  }
}

} }  // namespace iv::lv5
#endif  // IV_LV5_CONTEXT_H_
