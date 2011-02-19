#ifndef _IV_LV5_PROPERTY_H_
#define _IV_LV5_PROPERTY_H_
#include <cassert>
#include "lv5/jsval.h"

namespace iv {
namespace lv5 {
namespace detail {
}  // namespace iv::lv5::detail

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
    EMPTY = 256,
    UNDEF_VALUE = 512,
    UNDEF_GETTER = 1024,
    UNDEF_SETTER = 2048
  };

  enum DataDescriptorTag {
    kDataDescripter = 0
  };
  enum AccessorDescriptorTag {
    kAccessorDescriptor = 0
  };
  enum GenericDescriptorTag {
    kGenericDescriptor = 0
  };
  enum AccessorDescriptorGetterTag {
    kAccessorDescriptorGetter = 0
  };
  enum AccessorDescriptorSetterTag {
    kAccessorDescriptorSetter = 0
  };

  static const int kDefaultAttr =
      UNDEF_WRITABLE | UNDEF_ENUMERABLE | UNDEF_CONFIGURABLE |
      UNDEF_VALUE | UNDEF_GETTER | UNDEF_SETTER;
  static const int kTypeMask = DATA | ACCESSOR;
  static const int kDataAttrField = WRITABLE | ENUMERABLE | CONFIGURABLE;

  PropertyDescriptor(JSUndefinedKeywordType val)  // NOLINT
    : attrs_(kDefaultAttr | EMPTY),
      value_() {
  }

  PropertyDescriptor()
    : attrs_(kDefaultAttr | EMPTY),
      value_() {
  }

  // copy
  PropertyDescriptor(const PropertyDescriptor& rhs)
    : attrs_(rhs.attrs_),
      value_(rhs.value_) {
  }

  int attrs() const {
    return attrs_;
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

  inline bool IsGenericDescriptor() const {
    return (!(attrs_ & DATA)) && (!(attrs_ & ACCESSOR)) && (!(attrs_ & EMPTY));
  }

  inline bool IsEmpty() const {
    return attrs_ & EMPTY;
  }

  inline const DataDescriptor* AsDataDescriptor() const;

  inline DataDescriptor* AsDataDescriptor();

  inline const AccessorDescriptor* AsAccessorDescriptor() const;

  inline AccessorDescriptor* AsAccessorDescriptor();

  inline bool IsEnumerable() const {
    return attrs_ & ENUMERABLE;
  }

  inline bool IsEnumerableAbsent() const {
    return attrs_ & UNDEF_ENUMERABLE;
  }

  inline void set_enumerable(bool val) {
    if (val) {
      attrs_ = (attrs_ & ~UNDEF_ENUMERABLE) | ENUMERABLE;
    } else {
      attrs_ = (attrs_ & ~UNDEF_ENUMERABLE) & ~ENUMERABLE;
    }
  }

  inline bool IsConfigurable() const {
    return attrs_ & CONFIGURABLE;
  }

  inline bool IsConfigurableAbsent() const {
    return attrs_ & UNDEF_CONFIGURABLE;
  }

  inline void set_configurable(bool val) {
    if (val) {
      attrs_ = (attrs_ & ~UNDEF_CONFIGURABLE) | CONFIGURABLE;
    } else {
      attrs_ = (attrs_ & ~UNDEF_CONFIGURABLE) & ~CONFIGURABLE;
    }
  }

  static this_type SetDefault(const PropertyDescriptor& prop);

  inline void set_data_descriptor(const JSVal& value);
  inline void set_data_descriptor();
  inline void set_accessor_descriptor(JSObject* get, JSObject* set);
  inline void set_accessor_descriptor_getter(JSObject* set);
  inline void set_accessor_descriptor_setter(JSObject* set);

  inline this_type& operator=(const this_type& rhs) {
    if (this != &rhs) {
      this_type(rhs).swap(*this);
    }
    return *this;
  }

  static bool Equals(const this_type& lhs, const this_type& rhs);

  static bool IsAbsent(const PropertyDescriptor& desc);

  inline friend void swap(this_type& lhs, this_type& rhs) {
    return lhs.swap(rhs);
  }

  inline void swap(this_type& rhs) {
    using std::swap;
    swap(attrs_, rhs.attrs_);
    swap(value_, rhs.value_);
  }

  static this_type Merge(const PropertyDescriptor& desc,
                         const PropertyDescriptor& current);

 protected:
  PropertyDescriptor(DataDescriptorTag tag,
                     const JSVal& val, int attrs)
    : attrs_(attrs | DATA | UNDEF_GETTER | UNDEF_SETTER),
      value_() {
    value_.data_ = val.Layout();
  }

  PropertyDescriptor(AccessorDescriptorTag tag,
                     JSObject* getter, JSObject* setter,
                     int attrs)
    : attrs_(attrs | ACCESSOR | UNDEF_VALUE),
      value_() {
    value_.accessor_.getter_ = getter;
    value_.accessor_.setter_ = setter;
  }

  PropertyDescriptor(AccessorDescriptorGetterTag tag,
                     JSObject* getter, int attrs)
    : attrs_(attrs | ACCESSOR | UNDEF_VALUE | UNDEF_SETTER),
      value_() {
    value_.accessor_.getter_ = getter;
    value_.accessor_.setter_ = NULL;
  }

  PropertyDescriptor(AccessorDescriptorSetterTag tag,
                     JSObject* setter, int attrs)
    : attrs_(attrs | ACCESSOR | UNDEF_VALUE | UNDEF_GETTER),
      value_() {
    value_.accessor_.getter_ = NULL;
    value_.accessor_.setter_ = setter;
  }

  PropertyDescriptor(GenericDescriptorTag tag, int attrs)
    : attrs_(attrs | UNDEF_VALUE | UNDEF_GETTER | UNDEF_SETTER),
      value_() {
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
  AccessorDescriptor(JSObject* get, JSObject* set, int attrs)
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
  inline bool IsGetterAbsent() const {
    return attrs_ & UNDEF_GETTER;
  }
  inline bool IsSetterAbsent() const {
    return attrs_ & UNDEF_SETTER;
  }
  void set_get(JSObject* getter) {
    value_.accessor_.getter_ = getter;
  }
  void set_set(JSObject* setter) {
    value_.accessor_.setter_ = setter;
  }
};

class DataDescriptor: public PropertyDescriptor {
 public:
  DataDescriptor(const JSVal& value, int attrs)
     : PropertyDescriptor(kDataDescripter, value, attrs) {
  }
  DataDescriptor(int attrs)
     : PropertyDescriptor(kDataDescripter, JSUndefined, attrs | UNDEF_VALUE) {
  }
  int type() const {
    return DATA;
  }
  JSVal value() const {
    return value_.data_;
  }
  void set_value(const JSVal& val) {
    value_.data_ = val.Layout();
  }

  inline bool IsValueAbsent() const {
    return attrs_ & UNDEF_VALUE;
  }

  inline bool IsWritable() const {
    return attrs_ & WRITABLE;
  }

  inline bool IsWritableAbsent() const {
    return attrs_ & UNDEF_WRITABLE;
  }

  inline void set_writable(bool val) {
    if (val) {
      attrs_ = (attrs_ & ~UNDEF_WRITABLE) | WRITABLE;
    } else {
      attrs_ = (attrs_ & ~UNDEF_WRITABLE) & ~WRITABLE;
    }
  }
};

class GenericDescriptor : public PropertyDescriptor {
 public:
  GenericDescriptor(int attrs)
     : PropertyDescriptor(kGenericDescriptor, attrs) {
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

inline void PropertyDescriptor::set_data_descriptor(const JSVal& value) {
  attrs_ |= DATA;
  attrs_ &= ~ACCESSOR;
  attrs_ &= ~UNDEF_VALUE;
  attrs_ |= UNDEF_GETTER;
  attrs_ |= UNDEF_SETTER;
  value_.data_ = value.Layout();
}

inline void PropertyDescriptor::set_accessor_descriptor(JSObject* get, JSObject* set) {
  attrs_ &= ~DATA;
  attrs_ |= ACCESSOR;
  attrs_ |= UNDEF_VALUE;
  attrs_ &= ~UNDEF_GETTER;
  attrs_ &= ~UNDEF_SETTER;
  value_.accessor_.getter_ = get;
  value_.accessor_.setter_ = set;
}

inline void PropertyDescriptor::set_accessor_descriptor_getter(JSObject* get) {
  attrs_ &= ~DATA;
  attrs_ |= ACCESSOR;
  attrs_ |= UNDEF_VALUE;
  attrs_ &= ~UNDEF_GETTER;
  attrs_ &= ~UNDEF_SETTER;
  value_.accessor_.getter_ = get;
}

inline void PropertyDescriptor::set_accessor_descriptor_setter(JSObject* set) {
  attrs_ &= ~DATA;
  attrs_ |= ACCESSOR;
  attrs_ |= UNDEF_VALUE;
  attrs_ &= ~UNDEF_GETTER;
  attrs_ &= ~UNDEF_SETTER;
  value_.accessor_.setter_ = set;
}

inline PropertyDescriptor PropertyDescriptor::SetDefault(
    const PropertyDescriptor& prop) {
  this_type result(prop);
  if (prop.IsConfigurableAbsent()) {
    result.set_configurable(false);
  }
  if (prop.IsEnumerableAbsent()) {
    result.set_enumerable(false);
  }
  if (prop.IsDataDescriptor()) {
    const DataDescriptor* const data = prop.AsDataDescriptor();
    if (data->IsValueAbsent()) {
      result.set_data_descriptor(JSUndefined);
    } else {
      result.set_data_descriptor(data->value());
    }
    if (data->IsWritableAbsent()) {
      result.AsDataDescriptor()->set_writable(false);
    }
  } else if (prop.IsAccessorDescriptor()) {
    const AccessorDescriptor* const accs = prop.AsAccessorDescriptor();
    result.set_accessor_descriptor(accs->get(), accs->set());
  } else {
    assert(prop.IsGenericDescriptor());
    result.set_data_descriptor(JSUndefined);
    result.AsDataDescriptor()->set_writable(false);
  }
  return result;
}


inline bool PropertyDescriptor::Equals(const PropertyDescriptor& lhs,
                                       const PropertyDescriptor& rhs) {
  if (lhs.IsConfigurableAbsent() || lhs.IsConfigurable() != rhs.IsConfigurable()) {
    return false;
  }
  if (!lhs.IsEnumerableAbsent() || lhs.IsEnumerable() != rhs.IsEnumerable()) {
    return false;
  }
  if (lhs.type() != rhs.type()) {
    return false;
  }
  if (lhs.IsDataDescriptor()) {
    if (!lhs.AsDataDescriptor()->IsWritableAbsent() ||
        lhs.AsDataDescriptor()->IsWritable() != rhs.AsDataDescriptor()->IsWritable()) {
      return false;
    }
    return SameValue(lhs.value_.data_, rhs.value_.data_);
  } else if (lhs.IsAccessorDescriptor()) {
    return lhs.value_.accessor_.getter_ == rhs.value_.accessor_.getter_ &&
        lhs.value_.accessor_.setter_ == rhs.value_.accessor_.setter_;
  } else {
    assert(lhs.IsGenericDescriptor());
    return true;
  }
}

inline PropertyDescriptor PropertyDescriptor::Merge(
    const PropertyDescriptor& desc,
    const PropertyDescriptor& current) {
  PropertyDescriptor result(current);
  if (!desc.IsConfigurableAbsent()) {
    result.set_configurable(desc.IsConfigurable());
  }
  if (!desc.IsEnumerableAbsent()) {
    result.set_enumerable(desc.IsEnumerable());
  }
  if (desc.IsDataDescriptor()) {
    const DataDescriptor* const data = desc.AsDataDescriptor();
    if (!data->IsValueAbsent()) {
      result.set_data_descriptor(data->value());
    }
    if (!data->IsWritableAbsent()) {
      result.AsDataDescriptor()->set_writable(data->IsWritable());
    }
  } else if (desc.IsAccessorDescriptor()) {
    const AccessorDescriptor* const accs = desc.AsAccessorDescriptor();
    if (accs->IsGetterAbsent()) {
      result.set_accessor_descriptor_setter(accs->set());
    } else if (accs->IsSetterAbsent()) {
      result.set_accessor_descriptor_getter(accs->get());
    } else {
      result.set_accessor_descriptor(accs->get(), accs->set());
    }
  }
  return result;
}

inline bool PropertyDescriptor::IsAbsent(const PropertyDescriptor& desc) {
  return
      desc.IsConfigurableAbsent() &&
      desc.IsEnumerableAbsent() &&
      desc.IsGenericDescriptor();
}


} }  // namespace iv::lv5
#endif  // _IV_LV5_PROPERTY_H_
