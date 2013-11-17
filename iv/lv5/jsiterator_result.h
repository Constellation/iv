#ifndef IV_LV5_JSITERATOR_RESULT_H_
#define IV_LV5_JSITERATOR_RESULT_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/property.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/map.h>
#include <iv/lv5/slot.h>
#include <iv/lv5/class.h>
#include <iv/lv5/context.h>
namespace iv {
namespace lv5 {

class JSIteratorResult {
 public:
  enum Field {
    VALUE = 0,
    DONE = 1
  };

  static JSObject* New(Context* ctx, JSVal value, bool done) {
    JSObject* result =
        JSObject::New(ctx, ctx->global_data()->iterator_result_map());
    result->Direct(VALUE) = value;
    result->Direct(DONE) = JSVal::Bool(done);
    return result;
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSITERATOR_RESULT_H_
