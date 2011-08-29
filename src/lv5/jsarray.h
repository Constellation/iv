#ifndef _IV_LV5_JSARRAY_H_
#define _IV_LV5_JSARRAY_H_
#include <cstdlib>
#include <limits>
#include <vector>
#include <set>
#include <algorithm>
#include "conversions.h"
#include "lv5/error_check.h"
#include "lv5/gc_template.h"
#include "lv5/property.h"
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/map.h"
#include "lv5/slot.h"
#include "lv5/class.h"
#include "lv5/context_utils.h"
#include "lv5/object_utils.h"
#include "lv5/railgun/fwd.h"
namespace iv {
namespace lv5 {
namespace detail {

static const uint32_t kMaxVectorSize = 10000;

static bool IsDefaultDescriptor(const PropertyDescriptor& desc) {
  // only accept
  // { enumrable: true, configurable: true, writable:true, value: VAL }
  if (!desc.IsEnumerable()) {
    return false;
  }
  if (!desc.IsConfigurable()) {
    return false;
  }
  if (desc.IsAccessorDescriptor()) {
    return false;
  }
  if (desc.IsDataDescriptor()) {
    const DataDescriptor* const data = desc.AsDataDescriptor();
    return data->IsWritable();
  }
  return true;
}

static bool IsAbsentDescriptor(const PropertyDescriptor& desc) {
  if (!desc.IsEnumerable() && !desc.IsEnumerableAbsent()) {
    // explicitly not enumerable
    return false;
  }
  if (!desc.IsConfigurable() && !desc.IsConfigurableAbsent()) {
    // explicitly not configurable
    return false;
  }
  if (desc.IsAccessorDescriptor()) {
    return false;
  }
  if (desc.IsDataDescriptor()) {
    const DataDescriptor* const data = desc.AsDataDescriptor();
    return data->IsWritable() || data->IsWritableAbsent();
  }
  return true;
}

static DescriptorSlot::Data<uint32_t>
DescriptorToArrayLengthSlot(const PropertyDescriptor& desc) {
  assert(desc.IsDataDescriptor());
  const JSVal val = desc.AsDataDescriptor()->value();
  assert(val.IsNumber());
  const uint32_t res = val.GetUInt32();
  return DescriptorSlot::Data<uint32_t>(
      res,
      desc.attrs());
}

}  // namespace iv::lv5::detail

class Context;

class JSArray : public JSObject {
 public:
  friend class railgun::VM;
  typedef GCMap<uint32_t, JSVal>::type SparseArray;

  JSArray(Context* ctx, uint32_t len)
    : JSObject(context::GetArrayMap(ctx)),
      vector_((len <= detail::kMaxVectorSize) ? len : 4, JSEmpty),
      map_(NULL),
      dense_(true),
      length_(len, PropertyDescriptor::WRITABLE) {
  }

  uint32_t GetLength() const {
    return length_.value();
  }

  bool GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
    if (symbol::IsArrayIndexSymbol(name)) {
      const uint32_t index = symbol::GetIndexFromSymbol(name);
      if (detail::kMaxVectorSize > index) {
        // this target included in vector (if dense array)
        if (vector_.size() > index) {
          const JSVal& val = vector_[index];
          if (!val.IsEmpty()) {
            // current is target
            slot->set_descriptor(
                DataDescriptor(val,
                               PropertyDescriptor::ENUMERABLE |
                               PropertyDescriptor::CONFIGURABLE |
                               PropertyDescriptor::WRITABLE));
            return true;
          } else if (dense_) {
            // if dense array, target is undefined
            return false;
          }
        }
      } else {
        // target is index and included in map
        if (map_) {
          const SparseArray::const_iterator it = map_->find(index);
          if (it != map_->end()) {
            // target is found
            slot->set_descriptor(
                DataDescriptor(it->second,
                               PropertyDescriptor::ENUMERABLE |
                               PropertyDescriptor::CONFIGURABLE |
                               PropertyDescriptor::WRITABLE));
            return true;
          } else if (dense_) {
            // if target is not found and dense array,
            // target is undefined
            return false;
          }
        } else if (dense_) {
          // if map is none and dense array, target is undefined
          return false;
        }
      }
      return JSObject::GetOwnPropertySlot(ctx, name, slot);
    }
    if (name == symbol::length()) {
      slot->set_descriptor(length_);
      return true;
    }
    return JSObject::GetOwnPropertySlot(ctx, name, slot);
  }

#define REJECT(str)\
  do {\
    if (th) {\
      e->Report(Error::Type, str);\
    }\
    return false;\
  } while (0)

  bool DefineOwnProperty(Context* ctx,
                         Symbol name,
                         const PropertyDescriptor& desc,
                         bool th,
                         Error* e) {
    if (symbol::IsArrayIndexSymbol(name)) {
      const uint32_t index = symbol::GetIndexFromSymbol(name);
      // array index
      const uint32_t old_len = length_.value();
      if (index >= old_len && !length_.IsWritable()) {
        return false;
      }

      // define step
      Slot slot;
      if ((detail::IsDefaultDescriptor(desc) ||
           (index < old_len && detail::IsAbsentDescriptor(desc))) &&
           (dense_ || !JSObject::GetOwnPropertySlot(ctx, name, &slot))) {
        JSVal target;
        if (desc.IsDataDescriptor()) {
          target = desc.AsDataDescriptor()->value();
        } else {
          target = JSUndefined;
        }
        if (detail::kMaxVectorSize > index) {
          if (vector_.size() > index) {
            vector_[index] = target;
          } else {
            vector_.resize(index + 1, JSEmpty);
            vector_[index] = target;
          }
        } else {
          if (!map_) {
            map_ = new (GC) SparseArray();
          }
          (*map_)[index] = target;
        }
      } else {
        const bool succeeded =
            JSObject::DefineOwnProperty(
                ctx, name,
                desc, false, IV_LV5_ERROR_WITH(e, false));
        if (succeeded) {
          dense_ = false;
          if (detail::kMaxVectorSize > index) {
            if (vector_.size() > index) {
              vector_[index] = JSEmpty;
            }
          } else {
            if (map_) {
              const SparseArray::iterator it = map_->find(index);
              if (it != map_->end()) {
                map_->erase(it);
              }
            }
          }
        } else {
          REJECT("define own property failed");
        }
      }
      if (index >= old_len) {
        length_.set_value(index + 1);
      }
      return true;
    }
    if (name == symbol::length()) {
      if (desc.IsDataDescriptor()) {
        const DataDescriptor* const data = desc.AsDataDescriptor();
        if (data->IsValueAbsent()) {
          // GenericDescriptor
          // changing attribute [[Writable]] or TypeError.
          // [[Value]] not changed.
          //
          // length value is always not empty, so use
          // GetDefineOwnPropertyResult
          bool returned = false;
          if (IsDefineOwnPropertyAccepted(length_, desc, th, &returned, e)) {
            length_ =
                detail::DescriptorToArrayLengthSlot(
                    PropertyDescriptor::Merge(desc, length_));
          }
          return returned;
        }
        const double new_len_double =
            data->value().ToNumber(ctx, IV_LV5_ERROR_WITH(e, false));
        // length must be uint32_t
        const uint32_t new_len = core::DoubleToUInt32(new_len_double);
        if (new_len != new_len_double) {
          e->Report(Error::Range, "invalid array length");
          return false;
        }
        DataDescriptor new_len_desc(JSVal::UInt32(new_len), data->attrs());
        uint32_t old_len = length_.value();
        if (new_len >= old_len) {
          bool returned = false;
          if (IsDefineOwnPropertyAccepted(length_,
                                          new_len_desc,
                                          th, &returned, e)) {
            length_ =
                detail::DescriptorToArrayLengthSlot(
                    PropertyDescriptor::Merge(new_len_desc, length_));
          }
          return returned;
        }
        if (!length_.IsWritable()) {
          REJECT("\"length\" not writable");
        }
        const bool new_writable =
            new_len_desc.IsWritableAbsent() || new_len_desc.IsWritable();
        new_len_desc.set_writable(true);

        bool succeeded = false;
        if (IsDefineOwnPropertyAccepted(length_,
                                        new_len_desc, th, &succeeded, e)) {
          length_ =
              detail::DescriptorToArrayLengthSlot(
                  PropertyDescriptor::Merge(new_len_desc, length_));
        }
        if (!succeeded) {
          return false;
        }

        if (new_len < old_len) {
          if (dense_) {
            // dense array version
            CompactionToLength(new_len);
          } else if (old_len - new_len < (1 << 24)) {
            while (new_len < old_len) {
              old_len -= 1;
              // see Eratta
              const bool delete_succeeded =
                  JSArray::Delete(ctx, symbol::MakeSymbolFromIndex(old_len),
                                  false, IV_LV5_ERROR_WITH(e, false));
              if (!delete_succeeded) {
                new_len_desc.set_value(JSVal::UInt32(old_len + 1));
                if (!new_writable) {
                  new_len_desc.set_writable(false);
                }
                bool wasted = false;
                if (IsDefineOwnPropertyAccepted(length_,
                                                new_len_desc,
                                                false, &wasted, e)) {
                  length_ =
                      detail::DescriptorToArrayLengthSlot(
                          PropertyDescriptor::Merge(new_len_desc, length_));
                }
                IV_LV5_ERROR_GUARD_WITH(e, false);
                REJECT("shrink array failed");
              }
            }
          } else {
            std::vector<Symbol> keys;
            JSObject::GetOwnPropertyNames(ctx, &keys,
                                          JSObject::kIncludeNotEnumerable);
            std::set<uint32_t> ix;
            for (std::vector<Symbol>::const_iterator it = keys.begin(),
                 last = keys.end(); it != last; ++it) {
              if (symbol::IsArrayIndexSymbol(*it)) {
                ix.insert(symbol::GetIndexFromSymbol(*it));
              }
            }
            for (std::set<uint32_t>::const_reverse_iterator it = ix.rbegin(),
                 last = ix.rend(); it != last; --it) {
              if (*it < new_len) {
                break;
              }
              const bool delete_succeeded =
                  Delete(ctx, symbol::MakeSymbolFromIndex(*it), false, e);
              if (!delete_succeeded) {
                const uint32_t result_len = *it + 1;
                CompactionToLength(result_len);
                new_len_desc.set_value(JSVal::UInt32(result_len));
                if (!new_writable) {
                  new_len_desc.set_writable(false);
                }
                bool wasted = false;
                if (IsDefineOwnPropertyAccepted(length_,
                                                new_len_desc,
                                                false, &wasted, e)) {
                  length_ =
                      detail::DescriptorToArrayLengthSlot(
                          PropertyDescriptor::Merge(new_len_desc, length_));
                }
                IV_LV5_ERROR_GUARD_WITH(e, false);
                REJECT("shrink array failed");
              }
            }
            CompactionToLength(new_len);
          }
        }
        if (!new_writable) {
          const DataDescriptor target = DataDescriptor(
              PropertyDescriptor::WRITABLE |
              PropertyDescriptor::UNDEF_ENUMERABLE |
              PropertyDescriptor::UNDEF_CONFIGURABLE);
          bool wasted = false;
          if (IsDefineOwnPropertyAccepted(length_, target, false, &wasted, e)) {
            length_ =
                detail::DescriptorToArrayLengthSlot(
                    PropertyDescriptor::Merge(target, length_));
          }
        }
        return true;
      } else {
        // length is not configurable
        // so length is not changed
        bool returned = false;
        IsDefineOwnPropertyAccepted(length_, desc, th, &returned, e);
        return returned;
      }
    } else {
      return JSObject::DefineOwnProperty(ctx, name, desc, th, e);
    }
  }

#undef REJECT

  bool Delete(Context* ctx, Symbol name, bool th, Error* e) {
    if (symbol::IsArrayIndexSymbol(name)) {
      const uint32_t index = symbol::GetIndexFromSymbol(name);
      if (detail::kMaxVectorSize > index) {
        if (vector_.size() > index) {
          JSVal& val = vector_[index];
          if (!val.IsEmpty()) {
            val = JSEmpty;
            return true;
          } else if (dense_) {
            return true;
          }
        }
      } else {
        if (map_) {
          const SparseArray::iterator it = map_->find(index);
          if (it != map_->end()) {
            map_->erase(it);
            return true;
          } else if (dense_) {
            return true;
          }
        } else if (dense_) {
          return true;
        }
      }
      return JSObject::Delete(ctx, name, th, e);
    }
    if (symbol::length() == name) {
      if (th) {
        e->Report(Error::Type, "delete failed");
      }
      return false;
    }
    return JSObject::Delete(ctx, name, th, e);
  }

  void GetOwnPropertyNames(Context* ctx,
                           std::vector<Symbol>* vec,
                           EnumerationMode mode) const {
    uint32_t index = 0;
    if (length_.IsEnumerable() || (mode == kIncludeNotEnumerable)) {
      if (std::find(vec->begin(), vec->end(), symbol::length()) == vec->end()) {
        vec->push_back(symbol::length());
      }
    }
    for (JSVals::const_iterator it = vector_.begin(),
         last = vector_.end(); it != last; ++it, ++index) {
      if (!it->IsEmpty()) {
        const Symbol sym = symbol::MakeSymbolFromIndex(index);
        if (std::find(vec->begin(), vec->end(), sym) == vec->end()) {
          vec->push_back(sym);
        }
      }
    }
    if (map_) {
      for (SparseArray::const_iterator it = map_->begin(),
           last = map_->end(); it != last; ++it) {
        if (!it->second.IsEmpty()) {
          const Symbol sym = symbol::MakeSymbolFromIndex(it->first);
          if (std::find(vec->begin(), vec->end(), sym) == vec->end()) {
            vec->push_back(sym);
          }
        }
      }
    }
    JSObject::GetOwnPropertyNames(ctx, vec, mode);
  }

  static JSArray* New(Context* ctx) {
    JSArray* const ary = new JSArray(ctx, 0);
    ary->set_cls(JSArray::GetClass());
    ary->set_prototype(context::GetClassSlot(ctx, Class::Array).prototype);
    return ary;
  }

  static JSArray* New(Context* ctx, uint32_t n) {
    JSArray* const ary = new JSArray(ctx, n);
    ary->set_cls(JSArray::GetClass());
    ary->set_prototype(context::GetClassSlot(ctx, Class::Array).prototype);
    return ary;
  }

  static JSArray* NewPlain(Context* ctx) {
    return new JSArray(ctx, 0);
  }

  static const Class* GetClass() {
    static const Class cls = {
      "Array",
      Class::Array
    };
    return &cls;
  }

 private:
  void CompactionToLength(uint32_t length) {
    if (length > detail::kMaxVectorSize) {
      if (map_) {
        map_->erase(
            map_->upper_bound(length - 1), map_->end());
      }
    } else {
      if (map_) {
        map_->clear();
      }
      if (vector_.size() > length) {
        vector_.resize(length, JSEmpty);
      }
    }
  }

  // use VM only
  //   ReservedNew Reserve Set
  static JSArray* ReservedNew(Context* ctx, uint32_t len) {
    JSArray* ary = New(ctx, len);
    ary->Reserve(len);
    return ary;
  }

  void Reserve(uint32_t len) {
    if (len > detail::kMaxVectorSize) {
      // alloc map
      map_ = new (GC) SparseArray();
    }
  }

  void Set(uint32_t index, const JSVal& val) {
    if (detail::kMaxVectorSize > index) {
      vector_[index] = val;
    } else {
      (*map_)[index] = val;
    }
  }

  JSVals vector_;
  SparseArray* map_;
  bool dense_;
  DescriptorSlot::Data<uint32_t> length_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARRAY_H_
