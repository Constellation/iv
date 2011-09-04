// Slot object
// this is holder for lookup Map and determine making this lookup to PIC or not
//
// see also map.h
//
#ifndef IV_LV5_SLOT_H_
#define IV_LV5_SLOT_H_
#include "notfound.h"
#include "lv5/property.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
namespace iv {
namespace lv5 {

class Slot {
 public:
  Slot()
    : cacheable_(true),
      base_(NULL),
      desc_(JSUndefined),
      offset_(core::kNotFound) {
  }

  bool IsCacheable() const {
    return cacheable_;
  }

  void set_descriptor(const PropertyDescriptor& desc) {
    cacheable_ = false;
    base_ = NULL;
    desc_ = desc;
    offset_ = core::kNotFound;
  }

  void set_descriptor(const PropertyDescriptor& desc,
                      const JSObject* obj, std::size_t offset) {
    cacheable_ = cacheable_ && true;
    base_ = obj;
    desc_ = desc;
    offset_ = offset;
  }

  JSVal Get(Context* ctx, JSVal this_binding, Error* e) const {
    return desc_.Get(ctx, this_binding, e);
  }

  const JSObject* base() const {
    return base_;
  }

  const PropertyDescriptor& desc() const {
    return desc_;
  }

  std::size_t offset() const {
    return offset_;
  }

  void MakeUnCacheable() {
    cacheable_ = false;
  }

 private:
  bool cacheable_;
  const JSObject* base_;
  PropertyDescriptor desc_;
  std::size_t offset_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_SLOT_H_
