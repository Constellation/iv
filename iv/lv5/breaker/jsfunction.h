#ifndef IV_LV5_BREAKER_JSFUNCTION_H_
#define IV_LV5_BREAKER_JSFUNCTION_H_
#include <iv/ustringpiece.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsscript.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/jsarguments.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/breaker/context.h>
namespace iv {
namespace lv5 {
namespace breaker {

class JSFunction : public railgun::JSVMFunction {
 public:
  virtual JSVal Call(Arguments* args, JSVal this_binding, Error* e) {
    args->set_this_binding(this_binding);
    return breaker::Execute(static_cast<Context*>(args->ctx()), args, this, e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    Context* const ctx = static_cast<Context*>(args->ctx());
    JSObject* const obj = JSObject::New(ctx, code()->ConstructMap(ctx));
    const JSVal proto = Get(ctx, symbol::prototype(), IV_LV5_ERROR(e));
    if (proto.IsObject()) {
      obj->set_prototype(proto.object());
    }
    assert(args->IsConstructorCalled());
    const JSVal val = JSFunction::Call(args, obj, IV_LV5_ERROR(e));
    if (!val.IsObject()) {
      return obj;
    }
    return val;
  }

  static JSFunction* New(Context* ctx,
                         railgun::Code* code, JSEnv* env) {
    return new JSFunction(ctx, code, env);
  }

 private:
  JSFunction(Context* ctx,
             railgun::Code* code, JSEnv* env)
    : railgun::JSVMFunction(ctx, code, env) { }
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_JSFUNCTION_H_
