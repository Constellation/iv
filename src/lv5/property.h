#ifndef IV_LV5_PROPERTY_H_
#define IV_LV5_PROPERTY_H_
#include "lv5/property_fwd.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
#include "lv5/arguments.h"
namespace iv {
namespace lv5 {

JSVal PropertyDescriptor::Get(Context* ctx, JSVal this_binding, Error* e) const {
  if (IsDataDescriptor()) {
    return AsDataDescriptor()->value();
  } else {
    assert(IsAccessorDescriptor());
    JSObject* const getter = AsAccessorDescriptor()->get();
    if (getter) {
      ScopedArguments a(ctx, 0, IV_LV5_ERROR(e));
      return getter->AsCallable()->Call(&a, this_binding, e);
    } else {
      return JSUndefined;
    }
  }
}

} }  // namespace iv::lv5
#endif  // IV_LV5_PROPERTY_FWD_H_
