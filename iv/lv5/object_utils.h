#ifndef IV_LV5_OBJECT_UTILS_H_
#define IV_LV5_OBJECT_UTILS_H_
#include <iv/lv5/property.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {

// ECMA262 section 8.12.9 [[DefineOwnProperty]] step 5 and after
// this returns [[DefineOwnProperty]] descriptor is accepted or not,
// if you see return value of [[DefineOwnProperty]],
// see bool argument returned
//
// current is currently set PropertyDescriptor, and desc is which we try to set.
inline bool IsDefineOwnPropertyAccepted(const PropertyDescriptor& current,
                                        const PropertyDescriptor& desc,
                                        bool throw_error,
                                        bool* returned,
                                        Error* e) {
#define REJECT(str)\
  do {\
    if (throw_error) {\
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
  if (current.MergeWithNoEffect(desc)) {
    *returned = true;
    return false;
  }

  // step 7
  if (!current.IsConfigurable()) {
    if (desc.IsConfigurable()) {
      REJECT(
          "changing [[Configurable]] of unconfigurable property not allowed");
    }
    if (!desc.IsEnumerableAbsent() &&
        current.IsEnumerable() != desc.IsEnumerable()) {
      REJECT("changing [[Enumerable]] of unconfigurable property not allowed");
    }
  }

  // step 9
  if (desc.IsGenericDescriptor()) {
    // no further validation
  } else if (current.type() != desc.type()) {
    if (!current.IsConfigurable()) {
      REJECT("changing descriptor type of unconfigurable property not allowed");
    }
    assert(current.IsDataDescriptor() ?
           desc.IsAccessorDescriptor() : desc.IsDataDescriptor());
  } else {
    // step 10
    if (current.IsDataDescriptor()) {
      assert(desc.IsDataDescriptor());
      if (!current.IsConfigurable()) {
        if (!current.AsDataDescriptor()->IsWritable()) {
          const DataDescriptor* const data = desc.AsDataDescriptor();
          if (data->IsWritable()) {
            REJECT(
                "changing [[Writable]] of unconfigurable property not allowed");
          }
          // Value:Absent and Configurable/Enumerable/Writable test passed
          // Descriptor is unreachable because MergeWithNoEffect returns true
          assert(!data->IsValueAbsent());
          if (!JSVal::SameValue(current.AsDataDescriptor()->value(),
                                data->value())) {
            REJECT("changing [[Value]] of readonly property not allowed");
          }
        }
      }
    } else {
      // step 11
      assert(current.IsAccessorDescriptor());
      assert(desc.IsAccessorDescriptor());
      if (!current.IsConfigurable()) {
        const AccessorDescriptor* const lhs = current.AsAccessorDescriptor();
        const AccessorDescriptor* const rhs = desc.AsAccessorDescriptor();
        if ((!rhs->IsSetterAbsent() && (lhs->set() != rhs->set())) ||
            (!rhs->IsGetterAbsent() && (lhs->get() != rhs->get()))) {
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


} }  // namespace iv::lv5
#endif  // IV_LV5_OBJECT_UTILS_H_
