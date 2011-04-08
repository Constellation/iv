#ifndef _IV_LV5_JSARRAY_H_
#define _IV_LV5_JSARRAY_H_
#include "lv5/gc_template.h"
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/class.h"
#include "lv5/context_utils.h"
#include "lv5/railgun_fwd.h"
namespace iv {
namespace lv5 {
namespace detail {
  static const uint32_t kMaxVectorSize = 10000;
}  // namespace iv::lv5::detail
class Context;

class JSArray : public JSObject {
 public:
  friend class railgun::VM;
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

  static JSArray* New(Context* ctx) {
    JSArray* const ary = new JSArray(ctx, 0);
    const Class& cls = context::Cls(ctx, "Array");
    ary->set_class_name(cls.name);
    ary->set_prototype(cls.prototype);
    return ary;
  }

  static JSArray* New(Context* ctx, std::size_t n) {
    JSArray* const ary = new JSArray(ctx, n);
    const Class& cls = context::Cls(ctx, "Array");
    ary->set_class_name(cls.name);
    ary->set_prototype(cls.prototype);
    return ary;
  }

  static JSArray* NewPlain(Context* ctx) {
    return new JSArray(ctx, 0);
  }

  void Compaction(Context* ctx);
 private:
  void CompactionToLength(uint32_t length);
  static bool IsDefaultDescriptor(const PropertyDescriptor& desc);

  // use VM only
  //   ReservedNew Reserve Set
  static JSArray* ReservedNew(Context* ctx, uint32_t len) {
    JSArray* ary = New(ctx, len);
    ary->Reserve(len);
    return ary;
  }

  void Reserve(uint32_t len) {
    if (len > detail::kMaxVectorSize) {
      // alloc map
      map_ = new(GC)Map();
    }
  }

  void Set(uint32_t index, const JSVal& val) {
    if (detail::kMaxVectorSize > index) {
      vector_[index] = val;
    } else {
      (*map_)[index] = val;
    }
  }

  JSVals vector_;
  Map* map_;
  bool dense_;
  uint32_t length_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARRAY_H_
