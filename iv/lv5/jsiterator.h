#ifndef IV_LV5_JSITERATOR_H_
#define IV_LV5_JSITERATOR_H_
#include <iv/lv5/symbol.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/map.h>
namespace iv {
namespace lv5 {

class JSIterator : public JSObject {
 public:
  enum Field {
    NEXT = 0
  };

 private:
  template<JSAPI func>
  JSIterator(Context* ctx)
    : JSObject(ctx->global_data()->iterator_map()) {
    Direct(NEXT) = JSInlinedFunction<func, 0>::New(ctx, symbol::next());
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSITERATOR_H_
