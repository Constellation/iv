#include "property.h"
#include "jsval.h"
namespace iv {
namespace lv5 {

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

} }  // namespace iv::lv5
