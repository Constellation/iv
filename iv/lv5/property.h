#ifndef IV_LV5_PROPERTY_H_
#define IV_LV5_PROPERTY_H_
#include <iv/bit_cast.h>
#include <iv/lv5/property_fwd.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/accessor.h>
#include <iv/lv5/arguments.h>
namespace iv {
namespace lv5 {

inline  Accessor* StoredSlot::accessor() const {
  assert(attributes_.IsAccessor());
  return static_cast<Accessor*>(value_.cell());
}

inline JSVal StoredSlot::Get(Context* ctx, JSVal this_binding, Error* e) const {
  if (attributes_.IsData()) {
    return value_;
  }
  return accessor()->InvokeGetter(ctx, this_binding, e);
}

inline PropertyDescriptor StoredSlot::ToDescriptor() const {
  if (attributes().IsData()) {
    return PropertyDescriptor(value(), attributes());
  } else {
    assert(attributes_.IsAccessor());
    Accessor* ac = accessor();
    return PropertyDescriptor(ac->getter(), ac->setter(), attributes());
  }
}

// ECMA262 section 8.12.9 [[DefineOwnProperty]] step 5 and after
// this returns [[DefineOwnProperty]] descriptor is accepted or not,
// if you see return value of [[DefineOwnProperty]],
// see bool argument returned
//
// current is currently set PropertyDescriptor, and desc is which we try to set.
inline bool StoredSlot::IsDefineOwnPropertyAccepted(
    const PropertyDescriptor& desc, bool th, bool* returned, Error* e) const {
#define REJECT(str)\
  do {\
    if (th) {\
      e->Report(Error::Type, str);\
    }\
    *returned = false;\
    return false;\
  } while (0)

  // step 5
  if (desc.IsAbsent()) {
    *returned = true;
    return false;
  }

  // step 6
  if (MergeWithNoEffect(desc)) {
    *returned = true;
    return false;
  }

  // step 7
  if (!attributes().IsConfigurable()) {
    if (desc.IsConfigurable()) {
      REJECT(
          "changing [[Configurable]] of unconfigurable property not allowed");
    }
    if (!desc.IsEnumerableAbsent() &&
        attributes().IsEnumerable() != desc.IsEnumerable()) {
      REJECT("changing [[Enumerable]] of unconfigurable property not allowed");
    }
  }

  // step 9
  if (desc.IsGeneric()) {
    // no further validation
  } else if (attributes().IsData() != desc.IsData()) {
    if (!attributes().IsConfigurable()) {
      REJECT("changing descriptor type of unconfigurable property not allowed");
    }
    assert(attributes().IsData() ? desc.IsAccessor() : desc.IsData());
  } else {
    // step 10
    if (attributes().IsData()) {
      assert(desc.IsData());
      if (!attributes().IsConfigurable()) {
        if (!attributes().IsWritable()) {
          const DataDescriptor* const data = desc.AsDataDescriptor();
          if (data->IsWritable()) {
            REJECT(
                "changing [[Writable]] of unconfigurable property not allowed");
          }
          // Value:Absent and Configurable/Enumerable/Writable test passed
          // Descriptor is unreachable because MergeWithNoEffect returns true
          assert(!data->IsValueAbsent());
          if (!JSVal::SameValue(value(), data->value())) {
            REJECT("changing [[Value]] of readonly property not allowed");
          }
        }
      }
    } else {
      // step 11
      assert(attributes().IsAccessor());
      assert(desc.IsAccessor());
      if (!attributes().IsConfigurable()) {
        Accessor* lhs = accessor();
        const AccessorDescriptor* const rhs = desc.AsAccessorDescriptor();
        if ((!rhs->IsSetterAbsent() && (lhs->setter() != rhs->set())) ||
            (!rhs->IsGetterAbsent() && (lhs->getter() != rhs->get()))) {
          REJECT("changing [[Set]] or [[Get]] "
                 "of unconfigurable property not allowed");
        }
      }
    }
  }
  *returned = true;
  return true;
#undef REJECT
}

// if desc merged to current and has no effect, return true
inline bool StoredSlot::MergeWithNoEffect(
    const PropertyDescriptor& desc) const {
  if (!desc.IsConfigurableAbsent() &&
      desc.IsConfigurable() != attributes().IsConfigurable()) {
    return false;
  }
  if (!desc.IsEnumerableAbsent() && desc.IsEnumerable() != attributes().IsEnumerable()) {
    return false;
  }
  if (desc.type() != attributes().type()) {
    return false;
  }
  if (desc.IsData()) {
    const DataDescriptor* data = desc.AsDataDescriptor();
    if (!data->IsWritableAbsent() &&
        data->IsWritable() != attributes().IsWritable()) {
      return false;
    }
    if (data->IsValueAbsent()) {
      return true;
    }
    return JSVal::SameValue(data->value(), value());
  } else if (desc.IsAccessor()) {
    Accessor* ac = accessor();
    return desc.value_.accessor_.getter_ == ac->getter() &&
        desc.value_.accessor_.setter_ == ac->setter();
  } else {
    assert(desc.IsGeneric());
    return true;
  }
}


inline void StoredSlot::Merge(Context* ctx, const PropertyDescriptor& desc) {
  Attributes::External attr(attributes().raw());

  if (!desc.IsConfigurableAbsent()) {
    attr.set_configurable(desc.IsConfigurable());
  }
  if (!desc.IsEnumerableAbsent()) {
    attr.set_enumerable(desc.IsEnumerable());
  }

  if (desc.IsGeneric()) {
    attributes_ = Attributes::Safe::UnSafe(attr);
    return;
  }

  if (desc.IsData()) {
    attr.set_data();
    const DataDescriptor* const data = desc.AsDataDescriptor();
    if (!data->IsValueAbsent()) {
      value_ = data->value();
    }
    if (!data->IsWritableAbsent()) {
      attr.set_writable(data->IsWritable());
    }
    attributes_ = Attributes::Safe::UnSafe(attr);
  } else {
    assert(desc.IsAccessor());
    attr.set_accessor();
    const AccessorDescriptor* const accs = desc.AsAccessorDescriptor();

    Accessor* ac = NULL;
    if (attributes().IsAccessor()) {
      ac = accessor();
    } else {
      ac = Accessor::New(ctx, NULL, NULL);
      value_ = JSVal::Cell(ac);
    }

    if (accs->IsGetterAbsent()) {
      // Accessor Descriptor should have get or set
      assert(!accs->IsSetterAbsent());
      ac->set_setter(accs->set());
    } else if (accs->IsSetterAbsent()) {
      assert(!accs->IsGetterAbsent());
      ac->set_getter(accs->get());
    } else {
      ac->set_getter(accs->get());
      ac->set_setter(accs->set());
    }
    attributes_ = Attributes::Safe::UnSafe(attr);
  }
}

const DataDescriptor* PropertyDescriptor::AsDataDescriptor() const {
  assert(IsData());
  return core::BitCast<const DataDescriptor*>(this);
}

DataDescriptor* PropertyDescriptor::AsDataDescriptor() {
  assert(IsData());
  return core::BitCast<DataDescriptor*>(this);
}

const AccessorDescriptor* PropertyDescriptor::AsAccessorDescriptor() const {
  assert(IsAccessor());
  return core::BitCast<const AccessorDescriptor*>(this);
}

AccessorDescriptor* PropertyDescriptor::AsAccessorDescriptor() {
  assert(IsAccessor());
  return core::BitCast<AccessorDescriptor*>(this);
}

inline StoredSlot::StoredSlot(Context* ctx, const PropertyDescriptor& desc)
    : value_(JSUndefined),
      attributes_(Attributes::Safe::NotFound()) {
  Attributes::External attributes(ATTR::NONE);
  attributes.set_configurable(desc.IsConfigurable());
  attributes.set_enumerable(desc.IsEnumerable());
  if (desc.IsData()) {
    const DataDescriptor* const data = desc.AsDataDescriptor();
    if (!data->IsValueAbsent()) {
      value_ = data->value();
    }
    attributes.set_writable(data->IsWritable());
    attributes_ = Attributes::CreateData(attributes);
  } else if (desc.IsAccessor()) {
    const AccessorDescriptor* const ac = desc.AsAccessorDescriptor();
    Accessor* accessor = Accessor::New(ctx, ac->get(), ac->set());
    value_ = JSVal::Cell(accessor);
    attributes_ = Attributes::CreateAccessor(attributes);
  } else {
    assert(desc.IsGeneric());
    attributes_ = Attributes::CreateData(attributes);
  }
}

} }  // namespace iv::lv5
#endif  // IV_LV5_PROPERTY_FWD_H_
