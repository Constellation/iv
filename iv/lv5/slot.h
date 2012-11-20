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
#include <iv/lv5/jsfunction_fwd.h>
namespace iv {
namespace lv5 {

class Slot : public StoredSlot {
 public:
  static const uint32_t FLAG_USED = 1;
  static const uint32_t FLAG_CACHEABLE = 2;
  static const uint32_t FLAG_INIT = FLAG_CACHEABLE;

  Slot()
    : StoredSlot(JSEmpty, Attributes::Safe::NotFound()),
      base_(NULL),
      offset_(UINT32_MAX),
      flags_(FLAG_INIT) {
  }

  bool IsNotFound() const {
    return attributes().IsNotFound();
  }

  bool HasOffset() const {
    return offset_ != UINT32_MAX;
  }

  // see enumerable / configurable / writable flags
  bool IsStoreCacheable() const {
    return IsCacheable() && attributes().IsSimpleData();
  }

  bool IsLoadCacheable() const {
    return IsCacheable() && attributes().IsData();
  }

  void set(JSVal value, Attributes::Safe attributes, const JSObject* obj) {
    StoredSlot::set(value, attributes);
    MakeUsed();
    MakeUnCacheable();
    base_ = obj;
    offset_ = UINT32_MAX;
  }

  void set(JSVal value,
           Attributes::Safe attributes,
           const JSObject* obj, uint32_t offset) {
    StoredSlot::set(value, attributes);
    MakeUsed();
    base_ = obj;
    offset_ = offset;
  }

  void StringSet(JSVal value, Attributes::Safe attributes) {
    StoredSlot::set(value, attributes);
    MakeUsed();
    MakeUnCacheable();
    base_ = NULL;
    offset_ = UINT32_MAX;
  }

  const JSObject* base() const { return base_; }

  bool IsStringBase() const { return !base(); };

  uint32_t offset() const { return offset_; }

  inline void MakeUnCacheable() {
    flags_ &= (~FLAG_CACHEABLE);
  }

  inline void MakeUsed() {
    flags_ &= (~FLAG_USED);
  }

  void Clear() {
    StoredSlot::set(JSEmpty, Attributes::Safe::NotFound());
    flags_ = FLAG_INIT;
    base_ = NULL;
    offset_ = UINT32_MAX;
  }

  bool IsUsed() {
    return flags_ & FLAG_USED;
  }

 private:
  bool IsCacheable() const {
    return flags_ & FLAG_CACHEABLE;
  }

  const JSObject* base_;
  uint32_t offset_;
  uint32_t flags_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_SLOT_H_
