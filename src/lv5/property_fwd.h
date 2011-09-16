#ifndef IV_LV5_PROPERTY_FWD_H_
#define IV_LV5_PROPERTY_FWD_H_
#include "debug.h"
#include "lv5/jsval_fwd.h"
namespace iv {
namespace lv5 {

class JSObject;
class Context;
class Error;
class DataDescriptor;
class AccessorDescriptor;

class PropertyDescriptor {
 public:
  typedef PropertyDescriptor this_type;
  union PropertyLayout {
    struct Accessors {
      JSObject* getter_;
      JSObject* setter_;
    } accessor_;
    JSVal::value_type data_;
  };
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

  enum DataDescriptorTag { DATA_DESCRIPTOR };
  enum AccessorDescriptorTag { ACCESSOR_DESCRIPTOR };
  enum GenericDescriptorTag { GENERIC_DESCRIPTOR };
  enum AccessorDescriptorGetterTag { ACCESSOR_DESCRIPTOR_GETTER };
  enum AccessorDescriptorSetterTag { ACCESSOR_DESCRIPTOR_SETTER };

  static const int kDefaultAttr =
      UNDEF_WRITABLE | UNDEF_ENUMERABLE | UNDEF_CONFIGURABLE |
      UNDEF_VALUE | UNDEF_GETTER | UNDEF_SETTER;
  static const int kTypeMask = DATA | ACCESSOR;
  static const int kDataAttrField = WRITABLE | ENUMERABLE | CONFIGURABLE;

  PropertyDescriptor(detail::JSEmptyType val)  // NOLINT
    : attrs_(kDefaultAttr | EMPTY),
      value_() {
  }

  PropertyDescriptor()
    : attrs_(kDefaultAttr | EMPTY),
      value_() {
  }

  PropertyDescriptor(const PropertyDescriptor& rhs)
    : attrs_(rhs.attrs_),
      value_(rhs.value_) {
  }

  int attrs() const {
    return attrs_;
  }

  const PropertyLayout& GetLayout() const {
    return value_;
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

  inline bool IsAbsent() const;

  inline void set_configurable(bool val) {
    if (val) {
      attrs_ = (attrs_ & ~UNDEF_CONFIGURABLE) | CONFIGURABLE;
    } else {
      attrs_ = (attrs_ & ~UNDEF_CONFIGURABLE) & ~CONFIGURABLE;
    }
  }

  inline JSVal Get(Context* ctx, JSVal this_binding, Error* e) const;

  static inline this_type SetDefault(const PropertyDescriptor& prop);

  inline void set_data_descriptor(const JSVal& value);
  inline void set_data_descriptor();
  inline void set_accessor_descriptor(JSObject* get, JSObject* set);
  inline void set_accessor_descriptor_getter(JSObject* set);
  inline void set_accessor_descriptor_setter(JSObject* set);

  inline bool MergeWithNoEffect(const PropertyDescriptor& desc) const;

  inline friend void swap(this_type& lhs, this_type& rhs) {
    return lhs.swap(rhs);
  }

  inline void swap(this_type& rhs) {
    using std::swap;
    swap(attrs_, rhs.attrs_);
    swap(value_, rhs.value_);
  }

  static inline this_type Merge(const PropertyDescriptor& desc,
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
  PropertyLayout value_;
};

class AccessorDescriptor : public PropertyDescriptor {
 public:
  AccessorDescriptor(JSObject* get, JSObject* set, int attrs)
    : PropertyDescriptor(ACCESSOR_DESCRIPTOR, get, set, attrs) {
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
  explicit DataDescriptor(const JSVal& value, int attrs)
     : PropertyDescriptor(DATA_DESCRIPTOR, value, attrs) {
  }
  explicit DataDescriptor(int attrs)
     : PropertyDescriptor(DATA_DESCRIPTOR, JSUndefined, attrs | UNDEF_VALUE) {
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
  explicit GenericDescriptor(int attrs)
     : PropertyDescriptor(GENERIC_DESCRIPTOR, attrs) {
  }
};

template<typename T>
struct JSValConverter {
  JSVal operator()(const T& t) const {
    return JSVal(t);
  }
};

template<>
struct JSValConverter<uint32_t> {
  JSVal operator()(const uint32_t t) const {
    return JSVal::UInt32(t);
  }
};

class DescriptorSlot {
 public:
  template<typename T>
  class Data {
   public:
    Data(T value, int attrs)
      : value_(value),
        attrs_(attrs) {
    }

    operator DataDescriptor() const {
      return DataDescriptor(JSValConverter<T>()(value_), attrs_);
    }

    const T& value() const {
      return value_;
    }

    void set_value(T val) {
      value_ = val;
    }

    int attrs() const {
      return attrs_;
    }

    void set_attrs(int attrs) {
      attrs_ = attrs;
    }

    bool IsWritable() const {
      return attrs_ & PropertyDescriptor::WRITABLE;
    }

    bool IsConfigurable() const {
      return attrs_ & PropertyDescriptor::CONFIGURABLE;
    }

    bool IsEnumerable() const {
      return attrs_ & PropertyDescriptor::ENUMERABLE;
    }

   private:
    T value_;
    int attrs_;
  };
};

} }  // namespace iv::lv5
#endif  // IV_LV5_PROPERTY_FWD_H_
