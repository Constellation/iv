// Slot object
// this is holder for lookup Map and determine making this lookup to PIC or not
//
// see also map.h
#ifndef IV_LV5_SLOT_H_
#define IV_LV5_SLOT_H_
#include <iv/notfound.h>
#include <iv/lv5/property.h>
#include <iv/lv5/attributes.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/accessor.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction.h>
namespace iv {
namespace lv5 {

class Slot : public StoredSlot {
 public:
  Slot()
    : StoredSlot(JSEmpty, Attributes::Safe::NotFound()),
      cacheable_(true),
      base_(NULL),
      offset_(core::kNotFound) {
  }

  bool IsNotFound() const {
    return attributes().IsNotFound();
  }

  bool HasOffset() const {
    return offset_ != UINT32_MAX;
  }

  bool IsStoreCacheable() const {
    return IsCacheable() && attributes().IsSimpleData();
  }

  bool IsLoadCacheable() const {
    return IsCacheable() && attributes().IsData();
  }

  void set(JSVal value, Attributes::Safe attributes, const JSObject* obj) {
    StoredSlot::set(value, attributes);
    cacheable_ = false;
    base_ = obj;
    offset_ = UINT32_MAX;
  }

  void set(JSVal value,
           Attributes::Safe attributes,
           const JSObject* obj, uint32_t offset) {
    StoredSlot::set(value, attributes);
    cacheable_ = cacheable_ && true;
    base_ = obj;
    offset_ = offset;
  }

  const JSObject* base() const { return base_; }

  uint32_t offset() const { return offset_; }

  inline void MakeUnCacheable() {
    cacheable_ = false;
  }

 private:
  bool IsCacheable() const {
    return cacheable_;
  }

  bool cacheable_;
  const JSObject* base_;
  uint32_t offset_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_SLOT_H_
