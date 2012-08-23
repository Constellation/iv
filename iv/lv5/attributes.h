#ifndef IV_LV5_ATTRIBUTES_H_
#define IV_LV5_ATTRIBUTES_H_
namespace iv {
namespace lv5 {

class Attributes {
 public:
  enum Attribute {
    NONE = 0,
    WRITABLE = 1,
    ENUMERABLE = 2,
    CONFIGURABLE = 4,
    DATA = 8,
    ACCESSOR = 16,
    EMPTY = 32,
    UNDEF_WRITABLE = 64,
    UNDEF_ENUMERABLE = 128,
    UNDEF_CONFIGURABLE = 256,
    UNDEF_VALUE = 512,
    UNDEF_GETTER = 1024,
    UNDEF_SETTER = 2048,

    // short options
    N = NONE,
    W = WRITABLE,
    E = ENUMERABLE,
    C = CONFIGURABLE
  };

  typedef uint32_t Raw;

  static const Raw TYPE_MASK = DATA | ACCESSOR;
  static const Raw DATA_ATTR_MASK = DATA | WRITABLE | ENUMERABLE | CONFIGURABLE;
  static const Raw ACCESSOR_ATTR_MASK = ACCESSOR | ENUMERABLE | CONFIGURABLE;

  static const Raw DEFAULT =
      UNDEF_WRITABLE | UNDEF_ENUMERABLE | UNDEF_CONFIGURABLE |
      UNDEF_VALUE | UNDEF_GETTER | UNDEF_SETTER;
  static const Raw UNDEFS = EMPTY | DEFAULT;

  static const Raw BOTH = CONFIGURABLE | ENUMERABLE;

  static bool IsStored(Raw attributes) {
    // undef attributes should not be here
    if (attributes & UNDEFS) {
      return false;
    }
    if (attributes & DATA) {
      return !(attributes & ACCESSOR);
    }
    if (attributes & ACCESSOR) {
      return !(attributes & WRITABLE);
    }
    return false;
  }

  static Raw RemoveUndefs(Raw attributes) {
    return attributes & (~UNDEFS);
  }

  class External {
   public:
    friend class Attributes;
    External(Raw attributes) : attributes_(attributes) { }  // NOLINT
    External() : attributes_(NONE) { }

    Raw type() const {
      return attributes_ & TYPE_MASK;
    }

    inline bool IsEnumerable() const {
      return attributes_ & ENUMERABLE;
    }

    inline bool IsEnumerableAbsent() const {
      return attributes_ & UNDEF_ENUMERABLE;
    }

    inline void set_enumerable(bool val) {
      if (val) {
        attributes_ = (attributes_ & ~UNDEF_ENUMERABLE) | ENUMERABLE;
      } else {
        attributes_ = (attributes_ & ~UNDEF_ENUMERABLE) & ~ENUMERABLE;
      }
    }

    inline bool IsConfigurable() const {
      return attributes_ & CONFIGURABLE;
    }

    inline bool IsConfigurableAbsent() const {
      return attributes_ & UNDEF_CONFIGURABLE;
    }

    inline void set_configurable(bool val) {
      if (val) {
        attributes_ = (attributes_ & ~UNDEF_CONFIGURABLE) | CONFIGURABLE;
      } else {
        attributes_ = (attributes_ & ~UNDEF_CONFIGURABLE) & ~CONFIGURABLE;
      }
    }

    inline bool IsData() const {
      return attributes_ & DATA;
    }

    void set_data() {
      attributes_ &= ~ACCESSOR;
      attributes_ |= DATA;
    }

    inline bool IsAccessor() const {
      return attributes_ & ACCESSOR;
    }

    void set_accessor() {
      attributes_ &= ~(DATA | WRITABLE);
      attributes_ |= ACCESSOR;
    }

    inline bool IsGeneric() const {
      return !(attributes_ & (DATA | ACCESSOR | EMPTY));
    }

    inline bool IsEmpty() const { return attributes_ & EMPTY; }

    inline bool IsWritable() const {
      return attributes_ & WRITABLE;
    }

    inline bool IsWritableAbsent() const {
      return attributes_ & UNDEF_WRITABLE;
    }

    inline void set_writable(bool val) {
      if (val) {
        attributes_ = (attributes_ & ~UNDEF_WRITABLE) | WRITABLE;
      } else {
        attributes_ = (attributes_ & ~UNDEF_WRITABLE) & ~WRITABLE;
      }
    }

    inline bool IsValueAbsent() const {
      return attributes_ & UNDEF_VALUE;
    }

    inline bool IsGetterAbsent() const {
      return attributes_ & UNDEF_GETTER;
    }

    inline bool IsSetterAbsent() const {
      return attributes_ & UNDEF_SETTER;
    }

    bool IsAbsent() const {
      return
          IsConfigurableAbsent() &&
          IsEnumerableAbsent() &&
          IsGeneric();
    }

    inline bool IsDefault() const {
      const Raw kDefault =
          (CONFIGURABLE | ENUMERABLE | DATA | WRITABLE);
      return (attributes_ & kDefault) == kDefault;
    }

    Raw raw() const { return attributes_; }

    inline friend bool operator==(External lhs, External rhs) {
      return lhs.raw() == rhs.raw();
    }

    inline friend bool operator!=(External lhs, External rhs) {
      return lhs.raw() != rhs.raw();
    }

   private:
    void FillEnumerableAndConfigurable() {
      if (IsConfigurableAbsent()) {
        assert(!(attributes_ & CONFIGURABLE));
        attributes_ &= ~UNDEF_CONFIGURABLE;
      }

      if (IsEnumerableAbsent()) {
        assert(!(attributes_ & ENUMERABLE));
        attributes_ &= ~UNDEF_ENUMERABLE;
      }
    }

    Raw attributes_;
  };

  class Safe : protected Attributes::External {
   public:
    friend class Attributes;
    typedef Attributes::External super_type;
    using super_type::IsEnumerable;
    using super_type::IsConfigurable;
    using super_type::IsData;
    using super_type::IsAccessor;
    using super_type::IsGeneric;
    using super_type::IsWritable;
    using super_type::raw;
    using super_type::type;

    static Safe NotFound() {
      return Safe();
    }

    bool IsNotFound() const { return raw() == NONE; }

    inline friend bool operator==(Safe lhs, Safe rhs) {
      return lhs.raw() == rhs.raw();
    }

    inline friend bool operator!=(Safe lhs, Safe rhs) {
      return lhs.raw() != rhs.raw();
    }

    bool IsSimpleData() const {
      // we can ignore ENUMERABLE
      const Raw value = DATA | WRITABLE;
      return (raw() & value) == value;
    }

    static Safe UnSafe(External attr) {
      return Safe(attr);
    }

   private:
    explicit Safe(Raw attr)
      : External(RemoveUndefs(attr)) {
      assert(Attributes::IsStored(raw()));
    }

    explicit Safe(External attr)
      : External(RemoveUndefs(attr.raw())) {
      assert(Attributes::IsStored(raw()));
    }

    Safe() : External() { }
  };

  static Safe CreateData(External attributes) {
    assert(!attributes.IsEmpty());
    assert(!attributes.IsAccessor());

    attributes.FillEnumerableAndConfigurable();
    attributes.set_data();

    if (attributes.IsWritableAbsent()) {
      assert(!attributes.IsWritable());
      attributes.set_writable(false);
    }
    return Safe(attributes);
  }

  static Safe CreateAccessor(External attributes) {
    assert(!attributes.IsEmpty());
    assert(!attributes.IsWritable());
    assert(!attributes.IsData());

    attributes.FillEnumerableAndConfigurable();
    attributes.set_accessor();
    return Safe(attributes);
  }

  class Object {
   public:
    static Safe Data() {
      return Attributes::CreateData(WRITABLE | ENUMERABLE | CONFIGURABLE);
    }

    static Safe Accessor() {
      return Attributes::CreateAccessor(ENUMERABLE | CONFIGURABLE);
    }
  };

  class String {
   public:
    static Safe Length() {
      return CreateData(NONE);
    }

    static Safe Indexed() {
      return CreateData(ENUMERABLE);
    }
  };

 private:
  static bool CheckUndefs(Raw attributes) {
    return attributes & UNDEFS;
  }
};

typedef Attributes ATTR;

} }  // namespace iv::lv5
#endif  // IV_LV5_ATTRIBUTES_H_
