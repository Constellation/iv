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
#include <iv/lv5/breaker/context_fwd.h>
namespace iv {
namespace lv5 {
namespace breaker {

class JSJITFunction : public railgun::JSVMFunction {
 public:
  virtual JSVal Call(Arguments* args, JSVal this_binding, Error* e) {
    args->set_this_binding(this_binding);
    return breaker::Execute(static_cast<Context*>(args->ctx()), args, this, e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    Context* const ctx = static_cast<Context*>(args->ctx());
    Map* map = construct_map(ctx, IV_LV5_ERROR(e));
    JSObject* const obj = JSObject::New(ctx, map);
    assert(args->IsConstructorCalled());
    const JSVal val = JSJITFunction::Call(args, obj, IV_LV5_ERROR(e));
    if (!val.IsObject()) {
      return obj;
    }
    return val;
  }

  static JSJITFunction* New(Context* ctx,
                            railgun::Code* code, JSEnv* env) {
    return new JSJITFunction(ctx, code, env);
  }

 private:
  JSJITFunction(Context* ctx,
                railgun::Code* code, JSEnv* env)
    : railgun::JSVMFunction(ctx, code, env) { }
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_JSFUNCTION_H_
