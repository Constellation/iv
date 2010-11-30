#ifndef _IV_LV5_PROPERTY_H_
#define _IV_LV5_PROPERTY_H_
#include <cassert>
#include "jsval.h"

namespace iv {
namespace lv5 {

class JSObject;
class DataDescriptor;
class AccessorDescriptor;

class PropertyDescriptor {
 public:
  typedef PropertyDescriptor this_type;
  enum Attribute {
    NONE = 0,
    WRITABLE = 1,
    ENUMERABLE = 2,
    CONFIGURABLE = 4,
    UNDEF_WRITABLE = 8,
    UNDEF_ENUMERABLE = 16,
    UNDEF_CONFIGURABLE = 32,
    DATA = 64,
    ACCESSOR = 128,
    EMPTY = 256
  };
  enum State {
    kFALSE = 0,
    kTRUE = 1,
    kUNDEF = 2
  };
  enum DataDescriptorTag {
    kDataDescripter = 0
  };
  enum AccessorDescriptorTag {
    kAccessorDescriptor = 0
  };

  static const int kDefaultAttr = UNDEF_WRITABLE |
      UNDEF_ENUMERABLE | UNDEF_CONFIGURABLE;
  static const int kTypeMask = DATA | ACCESSOR;
  static const int kAttrField = WRITABLE | ENUMERABLE | CONFIGURABLE;

  PropertyDescriptor(JSUndefinedKeywordType val)  // NOLINT
    : attrs_(kDefaultAttr | EMPTY),
      value_() {
  }

  PropertyDescriptor()
    : attrs_(kDefaultAttr | EMPTY),
      value_() {
  }

  PropertyDescriptor(int attr)
    : attrs_(attr | EMPTY),
      value_() {
  }

  PropertyDescriptor(const PropertyDescriptor& rhs)
    : attrs_(rhs.attrs_),
      value_(rhs.value_) {
  }

  int attrs() const {
    return attrs_;
  }

  void set_attrs(int attr) {
    attrs_ = attr;
  }

  int type() const {
    return attrs_ & kTypeMask;
  }

  inline bool IsDataDescriptor() const {
    return attrs_ & DATA;
  }

  inline bool IsAccessorDescriptor() const {
    return attrs_ & ACCESSOR;
  }

  inline const DataDescriptor* AsDataDescriptor() const;

  inline DataDescriptor* AsDataDescriptor();

  inline const AccessorDescriptor* AsAccessorDescriptor() const;

  inline AccessorDescriptor* AsAccessorDescriptor();

  inline bool IsWritable() const {
    return attrs_ & WRITABLE;
  }

  inline State Writable() const {
    return (attrs_ & WRITABLE) ?
        kTRUE : (attrs_ & UNDEF_WRITABLE) ? kUNDEF : kFALSE;
  }

  inline bool IsWritableAbsent() const {
    return attrs_ & UNDEF_WRITABLE;
  }

  inline void SetWritable(bool val) {
    if (val) {
      attrs_ = (attrs_ & ~UNDEF_WRITABLE) | WRITABLE;
    } else {
      attrs_ = (attrs_ & ~UNDEF_WRITABLE) & ~WRITABLE;
    }
  }

  inline bool IsEnumerable() const {
    return attrs_ & ENUMERABLE;
  }

  inline State Enumerable() const {
    return (attrs_ & ENUMERABLE) ?
        kTRUE : (attrs_ & UNDEF_ENUMERABLE) ? kUNDEF : kFALSE;
  }

  inline bool IsEnumerableAbsent() const {
    return attrs_ & UNDEF_ENUMERABLE;
  }

  inline void SetEnumerable(bool val) {
    if (val) {
      attrs_ = (attrs_ & ~UNDEF_ENUMERABLE) | ENUMERABLE;
    } else {
      attrs_ = (attrs_ & ~UNDEF_ENUMERABLE) & ~ENUMERABLE;
    }
  }

  inline bool IsConfigurable() const {
    return attrs_ & CONFIGURABLE;
  }

  inline State Configurable() const {
    return (attrs_ & CONFIGURABLE) ?
        kTRUE : (attrs_ & UNDEF_CONFIGURABLE) ? kUNDEF : kFALSE;
  }

  inline bool IsConfigurableAbsent() const {
    return attrs_ & UNDEF_CONFIGURABLE;
  }

  inline void SetConfigurable(bool val) {
    if (val) {
      attrs_ = (attrs_ & ~UNDEF_CONFIGURABLE) | CONFIGURABLE;
    } else {
      attrs_ = (attrs_ & ~UNDEF_CONFIGURABLE) & ~CONFIGURABLE;
    }
  }

  inline bool IsEmpty() const {
    return attrs_ & EMPTY;
  }

  inline this_type SetDefaultToAbsent() const {
    this_type result(*this);
    if (IsConfigurableAbsent()) {
      result.SetConfigurable(false);
    }
    if (IsEnumerableAbsent()) {
      result.SetEnumerable(false);
    }
    if (IsWritableAbsent()) {
      result.SetWritable(false);
    }
    return result;
  }

  inline this_type MergeAttrs(int attrs) const {
    this_type result(*this);
    if (IsConfigurableAbsent()) {
      result.SetConfigurable(attrs & CONFIGURABLE);
    }
    if (IsEnumerableAbsent()) {
      result.SetEnumerable(attrs & ENUMERABLE);
    }
    if (IsWritableAbsent()) {
      result.SetWritable(attrs & WRITABLE);
    }
    return result;
  }

  inline this_type& operator=(const this_type& rhs) {
    if (this != &rhs) {
      this_type(rhs).swap(*this);
    }
    return *this;
  }

  inline bool Equals(const this_type& rhs) const {
    if (!IsConfigurableAbsent() && rhs.IsConfigurable() != IsConfigurable()) {
      return false;
    }
    if (!IsEnumerableAbsent() && rhs.IsEnumerable() != IsEnumerable()) {
      return false;
    }
    if (!IsWritableAbsent() && rhs.IsWritable() != IsWritable()) {
      return false;
    }
    if (IsDataDescriptor()) {
      if (rhs.IsDataDescriptor()) {
        return SameValue(value_.data_, rhs.value_.data_);
      }
    } else {
      assert(IsAccessorDescriptor());
      if (rhs.IsAccessorDescriptor()) {
        return value_.accessor_.getter_ == rhs.value_.accessor_.getter_ &&
            value_.accessor_.setter_ == rhs.value_.accessor_.setter_;
      }
    }
    return false;
  }

  inline friend void swap(this_type& lhs, this_type& rhs) {
    return lhs.swap(rhs);
  }

  inline void swap(this_type& rhs) {
    using std::swap;
    swap(attrs_, rhs.attrs_);
    swap(value_, rhs.value_);
  }

 protected:
  PropertyDescriptor(DataDescriptorTag tag,
                     const JSVal& val, int attrs)
    : attrs_(attrs | DATA),
      value_() {
    value_.data_ = val.Layout();
  }

  PropertyDescriptor(AccessorDescriptorTag tag,
                     JSObject* getter, JSObject* setter,
                     int attrs)
    : attrs_(attrs | ACCESSOR),
      value_() {
    value_.accessor_.getter_ = getter;
    value_.accessor_.setter_ = setter;
  }

  int attrs_;
  union PropertyLayout {
    struct Accessors {
      JSObject* getter_;
      JSObject* setter_;
    } accessor_;
    JSVal::value_type data_;
  } value_;
};

class AccessorDescriptor : public PropertyDescriptor {
 public:
  AccessorDescriptor(JSObject* get, JSObject* set,
                     int attrs = kDefaultAttr)
    : PropertyDescriptor(kAccessorDescriptor, get, set, attrs) {
  }
  int type() const {
    return ACCESSOR;
  }
  JSObject* get() const {
    return value_.accessor_.getter_;
  }
  JSObject* set() const {
    return value_.accessor_.setter_;
  }
};

class DataDescriptor: public PropertyDescriptor {
 public:
  DataDescriptor(const JSVal& value,
                 int attrs = kDefaultAttr)
     : PropertyDescriptor(kDataDescripter, value, attrs) {
  }
  int type() const {
    return DATA;
  }
  JSVal data() const {
    return value_.data_;
  }
  void set_value(const JSVal& val) {
    value_.data_ = val.Layout();
  }
};

inline const DataDescriptor* PropertyDescriptor::AsDataDescriptor() const {
  assert(IsDataDescriptor());
  return static_cast<const DataDescriptor* const>(this);
}

inline DataDescriptor* PropertyDescriptor::AsDataDescriptor() {
  assert(IsDataDescriptor());
  return static_cast<DataDescriptor*>(this);
}

inline
const AccessorDescriptor* PropertyDescriptor::AsAccessorDescriptor() const {
  assert(IsAccessorDescriptor());
  return static_cast<const AccessorDescriptor*>(this);
}

inline AccessorDescriptor* PropertyDescriptor::AsAccessorDescriptor() {
  assert(IsAccessorDescriptor());
  return static_cast<AccessorDescriptor*>(this);
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_PROPERTY_H_
