#ifndef _IV_LV5_JSPROPERTY_H_
#define _IV_LV5_JSPROPERTY_H_
#include <algorithm>
#include <iostream>
#include <gc/gc_cpp.h>
#include "jsval.h"

namespace iv {
namespace lv5 {

class JSObject;
class JSVal;

class AccessorDescriptor;
class DataDescriptor;

class PropertyDescriptor : public gc {
 public:
  enum Attribute {
    NONE = 0,
    WRITABLE = 1,
    ENUMERABLE = 2,
    CONFIGURABLE = 4,
    UNDEF_WRITABLE = 8,
    UNDEF_ENUMERABLE = 16,
    UNDEF_CONFIGURABLE = 32
  };
  enum State {
    kFALSE = 0,
    kTRUE = 1,
    kUNDEF = 2
  };
  static const int kDefaultAttr = UNDEF_WRITABLE |
      UNDEF_ENUMERABLE | UNDEF_CONFIGURABLE;
  enum Type {
    DATA = 1,
    ACCESSOR = 2
  };
  virtual AccessorDescriptor* AsAccessorDescriptor() = 0;
  virtual const AccessorDescriptor* AsAccessorDescriptor() const = 0;
  virtual DataDescriptor* AsDataDescriptor() = 0;
  virtual const DataDescriptor* AsDataDescriptor() const = 0;
  bool IsDataDescriptor() const {
    return AsDataDescriptor();
  }
  bool IsAccessorDescriptor() const {
    return AsAccessorDescriptor();
  }
  virtual int type() const = 0;
  virtual PropertyDescriptor* clone() const = 0;
  int attrs() const {
    return attrs_;
  }
  void set_attrs(int attr) {
    attrs_ = attr;
  }
  inline bool IsWritable() const {
    return attrs_ & WRITABLE;
  }
  inline State Writable() const {
    return (attrs_ & WRITABLE) ? kTRUE : (attrs_ & UNDEF_WRITABLE) ? kUNDEF : kFALSE;
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
    return (attrs_ & ENUMERABLE) ? kTRUE : (attrs_ & UNDEF_ENUMERABLE) ? kUNDEF : kFALSE;
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
    return (attrs_ & CONFIGURABLE) ? kTRUE : (attrs_ & UNDEF_CONFIGURABLE) ? kUNDEF : kFALSE;
  }
  inline bool IsConfigurableAbsent() const {
    return attrs_ & UNDEF_CONFIGURABLE;
  }
  bool Equals(const PropertyDescriptor& prop) const;
  inline void SetConfigurable(bool val) {
    if (val) {
      attrs_ = (attrs_ & ~UNDEF_CONFIGURABLE) | CONFIGURABLE;
    } else {
      attrs_ = (attrs_ & ~UNDEF_CONFIGURABLE) & ~CONFIGURABLE;
    }
  }
  inline PropertyDescriptor* SetDefaultToAbsent() {
    if (IsConfigurableAbsent()) {
      SetConfigurable(false);
    }
    if (IsEnumerableAbsent()) {
      SetEnumerable(false);
    }
    if (IsWritableAbsent()) {
      SetWritable(false);
    }
    return this;
  }
  inline PropertyDescriptor* MergeAttrs(int attrs) {
    if (IsConfigurableAbsent()) {
      SetConfigurable(attrs & CONFIGURABLE);
    }
    if (IsEnumerableAbsent()) {
      SetEnumerable(attrs & ENUMERABLE);
    }
    if (IsWritableAbsent()) {
      SetWritable(attrs & WRITABLE);
    }
    return this;
  }
 protected:
  PropertyDescriptor() : attrs_(kDefaultAttr) { }
  explicit PropertyDescriptor(int attrs) : attrs_(attrs) { }
  void swap(PropertyDescriptor& rhs) {
    using std::swap;
    swap(attrs_, rhs.attrs_);
  }
  int attrs_;
};

class AccessorDescriptor : public PropertyDescriptor {
 public:
  AccessorDescriptor(JSObject* get, JSObject* set, int attrs)
    : PropertyDescriptor(attrs),
      get_(get),
      set_(set) {
  }
  AccessorDescriptor(JSObject* get, JSObject* set)
     : PropertyDescriptor(),
      get_(get),
      set_(set) {
  }
  AccessorDescriptor(const AccessorDescriptor& rhs)
    : PropertyDescriptor(rhs.attrs()),
      get_(rhs.get_),
      set_(rhs.set_) {
  }
  DataDescriptor* AsDataDescriptor() {
    return NULL;
  }
  const DataDescriptor* AsDataDescriptor() const {
    return NULL;
  }
  AccessorDescriptor* AsAccessorDescriptor() {
    return this;
  }
  const AccessorDescriptor* AsAccessorDescriptor() const {
    return this;
  }
  int type() const {
    return ACCESSOR;
  }
  PropertyDescriptor* clone() const {
    return new AccessorDescriptor(*this);
  }
  JSObject* get() const {
    return get_;
  }
  JSObject* set() const {
    return set_;
  }
  inline bool Equals(const AccessorDescriptor& desc) const {
    return get_ == desc.get_ && set_ == desc.set_;
  }

  friend void swap(AccessorDescriptor& lhs, AccessorDescriptor& rhs) {
    return lhs.swap(rhs);
  }
 private:
  void swap(AccessorDescriptor& rhs) {
    using std::swap;
    PropertyDescriptor::swap(rhs);
    swap(get_, rhs.get_);
    swap(set_, rhs.set_);
  }
  JSObject* get_;
  JSObject* set_;
};

class DataDescriptor: public PropertyDescriptor {
 public:
  DataDescriptor(const JSVal& value, int attrs)
     : PropertyDescriptor(attrs),
       value_(value) {
  }
  explicit DataDescriptor(const JSVal& value)
     : PropertyDescriptor(),
       value_(value) {
  }
  DataDescriptor(const DataDescriptor& rhs)
     : PropertyDescriptor(rhs.attrs()),
       value_(rhs.value_) {
  }
  DataDescriptor* AsDataDescriptor() {
    return this;
  }
  const DataDescriptor* AsDataDescriptor() const {
    return this;
  }
  AccessorDescriptor* AsAccessorDescriptor() {
    return NULL;
  }
  const AccessorDescriptor* AsAccessorDescriptor() const {
    return NULL;
  }
  int type() const {
    return DATA;
  }
  PropertyDescriptor* clone() const {
    return new DataDescriptor(*this);
  }
  const JSVal& value() const {
    return value_;
  }
  void set_value(const JSVal& val) {
    value_ = val;
  }
  bool Equals(const DataDescriptor& desc) const;
  template<typename T>
  void set_value(const T& value) {
    value_.set_value(value);
  }

  friend void swap(DataDescriptor& lhs, DataDescriptor& rhs) {
    return lhs.swap(rhs);
  }
 private:
  void swap(DataDescriptor& rhs) {
    using std::swap;
    PropertyDescriptor::swap(rhs);
    swap(value_, rhs.value_);
  }
  JSVal value_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSPROPERTY_H_
