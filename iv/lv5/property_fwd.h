#ifndef IV_LV5_PROPERTY_FWD_H_
#define IV_LV5_PROPERTY_FWD_H_
#include <iv/debug.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/attributes.h>
namespace iv {
namespace lv5 {

class JSObject;
class Context;
class Error;
class DataDescriptor;
class AccessorDescriptor;
class Slot;
class Accessor;

class PropertyDescriptor : protected Attributes::Interface {
 public:
  friend class StoredSlot;
  typedef PropertyDescriptor this_type;

  union PropertyLayout {
    struct Accessors {
      JSObject* getter_;
      JSObject* setter_;
    } accessor_;
    JSLayout data_;
  };

  enum DataDescriptorTag { DATA_DESCRIPTOR };
  enum AccessorDescriptorTag { ACCESSOR_DESCRIPTOR };
  enum GenericDescriptorTag { GENERIC_DESCRIPTOR };
  enum AccessorDescriptorGetterTag { ACCESSOR_DESCRIPTOR_GETTER };
  enum AccessorDescriptorSetterTag { ACCESSOR_DESCRIPTOR_SETTER };

  PropertyDescriptor(detail::JSEmptyType val)  // NOLINT
    : Attributes::Interface(ATTR::UNDEFS),
      value_() {
  }

  PropertyDescriptor()
    : Attributes::Interface(ATTR::UNDEFS),
      value_() {
  }

  PropertyDescriptor(const PropertyDescriptor& rhs)
    : Attributes::Interface(rhs),
      value_(rhs.value_) {
  }

  const PropertyLayout& GetLayout() const {
    return value_;
  }

  inline const DataDescriptor* AsDataDescriptor() const;

  inline DataDescriptor* AsDataDescriptor();

  inline const AccessorDescriptor* AsAccessorDescriptor() const;

  inline AccessorDescriptor* AsAccessorDescriptor();

  bool MergeWithNoEffect(const PropertyDescriptor& desc) const;

  using Attributes::Interface::IsEnumerable;
  using Attributes::Interface::IsEnumerableAbsent;
  using Attributes::Interface::set_enumerable;
  using Attributes::Interface::IsConfigurable;
  using Attributes::Interface::IsConfigurableAbsent;
  using Attributes::Interface::set_configurable;
  using Attributes::Interface::IsData;
  using Attributes::Interface::set_data;
  using Attributes::Interface::IsAccessor;
  using Attributes::Interface::set_accessor;
  using Attributes::Interface::IsGeneric;
  using Attributes::Interface::IsEmpty;
  using Attributes::Interface::IsWritable;
  using Attributes::Interface::IsWritableAbsent;
  using Attributes::Interface::set_writable;
  using Attributes::Interface::IsValueAbsent;
  using Attributes::Interface::IsGetterAbsent;
  using Attributes::Interface::IsSetterAbsent;
  using Attributes::Interface::IsAbsent;
  using Attributes::Interface::IsDefault;

 protected:
  PropertyLayout layout() const { return value_; }

  PropertyDescriptor(DataDescriptorTag tag,
                     JSVal val, Attributes::Raw attrs)
    : Attributes::Interface(attrs |
                            ATTR::DATA |
                            ATTR::UNDEF_GETTER |
                            ATTR::UNDEF_SETTER),
      value_() {
    value_.data_ = val;
  }

  PropertyDescriptor(AccessorDescriptorTag tag,
                     JSObject* getter, JSObject* setter,
                     Attributes::Raw attrs)
    : Attributes::Interface(attrs |
                            ATTR::ACCESSOR |
                            ATTR::UNDEF_VALUE |
                            ATTR::UNDEF_WRITABLE),
      value_() {
    value_.accessor_.getter_ = getter;
    value_.accessor_.setter_ = setter;
  }

  PropertyDescriptor(AccessorDescriptorGetterTag tag,
                     JSObject* getter, Attributes::Raw attrs)
    : Attributes::Interface(attrs |
                            ATTR::ACCESSOR |
                            ATTR::UNDEF_VALUE |
                            ATTR::UNDEF_SETTER |
                            ATTR::UNDEF_WRITABLE),
      value_() {
    value_.accessor_.getter_ = getter;
    value_.accessor_.setter_ = NULL;
  }

  PropertyDescriptor(AccessorDescriptorSetterTag tag,
                     JSObject* setter, Attributes::Raw attrs)
    : Attributes::Interface(attrs |
                            ATTR::ACCESSOR |
                            ATTR::UNDEF_VALUE |
                            ATTR::UNDEF_GETTER |
                            ATTR::UNDEF_WRITABLE),
      value_() {
    value_.accessor_.getter_ = NULL;
    value_.accessor_.setter_ = setter;
  }

  PropertyDescriptor(GenericDescriptorTag tag, Attributes::Raw attrs)
    : Attributes::Interface(attrs |
                            ATTR::UNDEF_VALUE |
                            ATTR::UNDEF_GETTER |
                            ATTR::UNDEF_SETTER |
                            ATTR::UNDEF_WRITABLE),
      value_() {
  }

  // raw
  PropertyDescriptor(PropertyLayout layout, Attributes::Safe attributes)
    : Attributes::Interface(attributes.raw()),
      value_() {
    value_ = layout;
  }

  PropertyLayout value_;
};

class StoredSlot {
 public:
  StoredSlot(JSVal value, Attributes::Safe attributes)
    : value_(value),
      attributes_(attributes) {
  }

  explicit StoredSlot(Context* ctx, const PropertyDescriptor& desc);

  Accessor* accessor() const;

  void set(JSVal value, Attributes::Safe attributes) {
    value_ = value;
    attributes_ = attributes;
  }

  JSVal Get(Context* ctx, JSVal this_binding, Error* e) const;

  PropertyDescriptor ToDescriptor() const;

  JSVal value() const { return value_; }

  void set_value(JSVal value) {
    value_ = value;
  }

  Attributes::Safe attributes() const { return attributes_; }

  void set_attributes(Attributes::Safe attributes) {
    attributes_ = attributes;
  }

  bool IsDefineOwnPropertyAccepted(
      const PropertyDescriptor& desc, bool th, bool* returned, Error* e) const;

  bool MergeWithNoEffect(const PropertyDescriptor& desc) const;

  void Merge(Context* ctx, const PropertyDescriptor& desc);
 private:
  JSVal value_;
  Attributes::Safe attributes_;
};

class AccessorDescriptor : public PropertyDescriptor {
 public:
  AccessorDescriptor(JSObject* get, JSObject* set, uint32_t attrs)
    : PropertyDescriptor(ACCESSOR_DESCRIPTOR, get, set, attrs) {
  }
  JSObject* get() const {
    return value_.accessor_.getter_;
  }
  JSObject* set() const {
    return value_.accessor_.setter_;
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
  explicit DataDescriptor(JSVal value, uint32_t attrs)
     : PropertyDescriptor(DATA_DESCRIPTOR, value, attrs) {
  }
  explicit DataDescriptor(uint32_t attrs)
     : PropertyDescriptor(DATA_DESCRIPTOR, JSUndefined,
                          attrs | ATTR::UNDEF_VALUE) {
  }
  JSVal value() const {
    return value_.data_;
  }
  void set_value(JSVal val) {
    value_.data_ = val;
  }
};

class GenericDescriptor : public PropertyDescriptor {
 public:
  explicit GenericDescriptor(uint32_t attrs)
     : PropertyDescriptor(GENERIC_DESCRIPTOR, attrs) {
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_PROPERTY_FWD_H_
