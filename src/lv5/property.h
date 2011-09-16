#ifndef IV_LV5_PROPERTY_H_
#define IV_LV5_PROPERTY_H_
#include "lv5/property_fwd.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
#include "lv5/arguments.h"
namespace iv {
namespace lv5 {

JSVal PropertyDescriptor::Get(Context* ctx, JSVal this_binding, Error* e) const {
  if (IsDataDescriptor()) {
    return AsDataDescriptor()->value();
  } else {
    assert(IsAccessorDescriptor());
    JSObject* const getter = AsAccessorDescriptor()->get();
    if (getter) {
      ScopedArguments a(ctx, 0, IV_LV5_ERROR(e));
      return getter->AsCallable()->Call(&a, this_binding, e);
    } else {
      return JSUndefined;
    }
  }
}

const DataDescriptor* PropertyDescriptor::AsDataDescriptor() const {
  assert(IsDataDescriptor());
  return static_cast<const DataDescriptor* const>(this);
}

DataDescriptor* PropertyDescriptor::AsDataDescriptor() {
  assert(IsDataDescriptor());
  return static_cast<DataDescriptor*>(this);
}

const AccessorDescriptor* PropertyDescriptor::AsAccessorDescriptor() const {
  assert(IsAccessorDescriptor());
  return static_cast<const AccessorDescriptor*>(this);
}

AccessorDescriptor* PropertyDescriptor::AsAccessorDescriptor() {
  assert(IsAccessorDescriptor());
  return static_cast<AccessorDescriptor*>(this);
}

void PropertyDescriptor::set_data_descriptor(const JSVal& value) {
  attrs_ |= ATTR::DATA;
  attrs_ &= ~ATTR::ACCESSOR;
  attrs_ &= ~ATTR::UNDEF_VALUE;
  attrs_ |= ATTR::UNDEF_GETTER;
  attrs_ |= ATTR::UNDEF_SETTER;
  value_.data_ = value.Layout();
}

void PropertyDescriptor::set_accessor_descriptor(JSObject* get, JSObject* set) {
  attrs_ &= ~ATTR::DATA;
  attrs_ |= ATTR::ACCESSOR;
  attrs_ |= ATTR::UNDEF_VALUE;
  attrs_ &= ~ATTR::UNDEF_GETTER;
  attrs_ &= ~ATTR::UNDEF_SETTER;
  value_.accessor_.getter_ = get;
  value_.accessor_.setter_ = set;
}

void PropertyDescriptor::set_accessor_descriptor_getter(JSObject* get) {
  if (IsDataDescriptor()) {
    value_.accessor_.setter_ = NULL;
  }
  attrs_ &= ~ATTR::DATA;
  attrs_ |= ATTR::ACCESSOR;
  attrs_ |= ATTR::UNDEF_VALUE;
  attrs_ &= ~ATTR::UNDEF_GETTER;
  attrs_ &= ~ATTR::UNDEF_SETTER;
  value_.accessor_.getter_ = get;
}

void PropertyDescriptor::set_accessor_descriptor_setter(JSObject* set) {
  if (IsDataDescriptor()) {
    value_.accessor_.getter_ = NULL;
  }
  attrs_ &= ~ATTR::DATA;
  attrs_ |= ATTR::ACCESSOR;
  attrs_ |= ATTR::UNDEF_VALUE;
  attrs_ &= ~ATTR::UNDEF_GETTER;
  attrs_ &= ~ATTR::UNDEF_SETTER;
  value_.accessor_.setter_ = set;
}

PropertyDescriptor PropertyDescriptor::SetDefault(
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

// if desc merged to current and has no effect, return true
bool PropertyDescriptor::MergeWithNoEffect(const PropertyDescriptor& desc) const {
  if (!desc.IsConfigurableAbsent() && desc.IsConfigurable() != IsConfigurable()) {
    return false;
  }
  if (!desc.IsEnumerableAbsent() && desc.IsEnumerable() != IsEnumerable()) {
    return false;
  }
  if (desc.type() != type()) {
    return false;
  }
  if (desc.IsDataDescriptor()) {
    if (!desc.AsDataDescriptor()->IsWritableAbsent() &&
        desc.AsDataDescriptor()->IsWritable() != AsDataDescriptor()->IsWritable()) {
      return false;
    }
    if (desc.AsDataDescriptor()->IsValueAbsent()) {
      return true;
    } else {
      return JSVal::SameValue(desc.value_.data_, value_.data_);
    }
  } else if (desc.IsAccessorDescriptor()) {
    return desc.value_.accessor_.getter_ == value_.accessor_.getter_ &&
        desc.value_.accessor_.setter_ == value_.accessor_.setter_;
  } else {
    assert(desc.IsGenericDescriptor());
    return true;
  }
}

PropertyDescriptor PropertyDescriptor::Merge(
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

bool PropertyDescriptor::IsAbsent() const {
  return
      IsConfigurableAbsent() &&
      IsEnumerableAbsent() &&
      IsGenericDescriptor();
}

} }  // namespace iv::lv5
#endif  // IV_LV5_PROPERTY_FWD_H_
