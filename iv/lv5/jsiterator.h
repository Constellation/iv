#ifndef IV_LV5_JSITERATOR_H_
#define IV_LV5_JSITERATOR_H_
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/map.h>
namespace iv {
namespace lv5 {

class JSIterator : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSIterator, Iterator)

  static JSIterator* New(Context* ctx) {
    JSIterator* const obj = new JSIterator(ctx, value);
    obj->set_cls(JSIterator::GetClass());
    obj->set_prototype(ctx->global_data()->object_prototype());
    return obj;
  }
 private:
  JSIterator(Context* ctx)
    : JSObject(Map::NewUnique(ctx)) {
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSITERATOR_H_
