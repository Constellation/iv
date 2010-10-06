#ifndef _IV_LV5_JSARRAY_H_
#define _IV_LV5_JSARRAY_H_
#include "gc-template.h"
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
                         const PropertyDescriptor* desc,
                         bool th,
                         JSErrorCode::Type* res);

  static JSArray* New(Context* ctx);

 private:
  std::size_t length_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARRAY_H_
