#ifndef _IV_LV5_JSARRAY_H_
#define _IV_LV5_JSARRAY_H_
#include "gc_template.h"
#include "jsval.h"
#include "jsobject.h"
namespace iv {
namespace lv5 {
namespace detail {
  static const uint32_t kMaxVectorSize = 10000;
}  // namespace iv::lv5::detail
class Context;
class JSArray : public JSObject {
 public:
  typedef GCVector<JSVal>::type Vector;
  typedef GCMap<uint32_t, JSVal>::type Map;

  JSArray(Context* ctx, std::size_t len);

  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;
  PropertyDescriptor GetOwnPropertyWithIndex(Context* ctx,
                                             uint32_t index) const;
  bool DefineOwnProperty(Context* ctx,
                         Symbol name,
                         const PropertyDescriptor& desc,
                         bool th,
                         Error* res);
  bool DefineOwnPropertyWithIndex(Context* ctx,
                                  uint32_t index,
                                  const PropertyDescriptor& desc,
                                  bool th,
                                  Error* res);

  bool Delete(Context* ctx, Symbol name, bool th, Error* res);
  bool DeleteWithIndex(Context* ctx, uint32_t index, bool th, Error* res);
  void GetOwnPropertyNames(Context* ctx,
                           std::vector<Symbol>* vec,
                           EnumerationMode mode) const;

  static JSArray* New(Context* ctx);
  static JSArray* New(Context* ctx, std::size_t n);
  static JSArray* NewPlain(Context* ctx);

 private:
  static bool IsDefaultDescriptor(const PropertyDescriptor& desc);

  Vector vector_;
  Map* map_;
  bool dense_;
  uint32_t length_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARRAY_H_
