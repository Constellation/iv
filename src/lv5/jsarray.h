#ifndef _IV_LV5_JSARRAY_H_
#define _IV_LV5_JSARRAY_H_
#include "gc_template.h"
#include "jsobject.h"
namespace iv {
namespace lv5 {
class Context;
class JSArray : public JSObject {
 public:
  typedef GCVector<JSVal>::type Vector;

  JSArray(Context* ctx, std::size_t len);

  bool DefineOwnProperty(Context* ctx,
                         Symbol name,
                         const PropertyDescriptor& desc,
                         bool th,
                         Error* res);

  static JSArray* New(Context* ctx);
  static JSArray* New(Context* ctx, std::size_t n);

 private:
  std::size_t length_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARRAY_H_
