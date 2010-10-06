#include "jsproperty.h"
#include "jsval.h"
#include "interpreter.h"
namespace iv {
namespace lv5 {

bool PropertyDescriptor::Equals(const PropertyDescriptor& prop) const {
  if (!IsConfigurableAbsent() && prop.IsConfigurable() != IsConfigurable()) {
    return false;
  }
  if (!IsEnumerableAbsent() && prop.IsEnumerable() != IsEnumerable()) {
    return false;
  }
  if (!IsWritableAbsent() && prop.IsWritable() != IsWritable()) {
    return false;
  }
  if (const DataDescriptor* const desc = AsDataDescriptor()) {
    if (const DataDescriptor* const target = prop.AsDataDescriptor()) {
      return desc->Equals(*target);
    } else {
      return false;
    }
  } else {
    if (const AccessorDescriptor* const target = prop.AsAccessorDescriptor()) {
      return AsAccessorDescriptor()->Equals(*target);
    } else {
      return false;
    }
  }
}

bool DataDescriptor::Equals(const DataDescriptor& desc) const {
  return Interpreter::SameValue(value_, desc.value_);
}

} }  // namespace iv::lv5
