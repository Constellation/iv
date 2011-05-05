#ifndef _IV_LV5_JSARRAY_H_
#define _IV_LV5_JSARRAY_H_
#include <cstdlib>
#include <limits>
#include <vector>
#include <set>
#include <algorithm>
#include <tr1/array>
#include "conversions.h"
#include "lv5/lv5.h"
#include "lv5/gc_template.h"
#include "lv5/property.h"
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/class.h"
#include "lv5/context_utils.h"
#include "lv5/symbol_checker.h"
#include "lv5/railgun_fwd.h"
namespace iv {
namespace lv5 {
namespace detail {

static const uint32_t kMaxVectorSize = 10000;

static bool IsDefaultDescriptor(const PropertyDescriptor& desc) {
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

}  // namespace iv::lv5::detail

class Context;

class JSArray : public JSObject {
 public:
  friend class railgun::VM;
  typedef GCMap<uint32_t, JSVal>::type Map;

  JSArray(Context* ctx, uint32_t len)
    : JSObject(),
      vector_((len <= detail::kMaxVectorSize) ? len : 4, JSEmpty),
      map_(NULL),
      dense_(true) {
    JSObject::DefineOwnProperty(
        ctx, context::length_symbol(ctx),
        DataDescriptor(JSVal::UInt32(len),
                       PropertyDescriptor::WRITABLE),
        false, NULL);
  }

  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const {
    uint32_t index;
    if (core::ConvertToUInt32(context::GetSymbolString(ctx, name), &index)) {
      return JSArray::GetOwnPropertyWithIndex(ctx, index);
    }
    return JSObject::GetOwnProperty(ctx, name);
  }

  PropertyDescriptor GetOwnPropertyWithIndex(Context* ctx,
                                             uint32_t index) const {
    if (detail::kMaxVectorSize > index) {
      // this target included in vector (if dense array)
      if (vector_.size() > index) {
        const JSVal& val = vector_[index];
        if (!val.IsEmpty()) {
          // current is target
          return DataDescriptor(val,
                                PropertyDescriptor::ENUMERABLE |
                                PropertyDescriptor::CONFIGURABLE |
                                PropertyDescriptor::WRITABLE);
        } else if (dense_) {
          // if dense array, target is undefined
          return JSUndefined;
        }
      }
    } else {
      // target is index and included in map
      if (map_) {
        const Map::const_iterator it = map_->find(index);
        if (it != map_->end()) {
          // target is found
          return DataDescriptor(it->second,
                                PropertyDescriptor::ENUMERABLE |
                                PropertyDescriptor::CONFIGURABLE |
                                PropertyDescriptor::WRITABLE);
        } else if (dense_) {
          // if target is not found and dense array,
          // target is undefined
          return JSUndefined;
        }
      } else if (dense_) {
        // if map is none and dense array, target is undefined
        return JSUndefined;
      }
    }
    if (const SymbolChecker check = SymbolChecker(ctx, index)) {
      return JSObject::GetOwnProperty(ctx, check.symbol());
    } else {
      // property not defined
      return JSUndefined;
    }
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
    uint32_t index;
    if (core::ConvertToUInt32(context::GetSymbolString(ctx, name), &index)) {
      return JSArray::DefineOwnPropertyWithIndex(ctx, index, desc, th, e);
    }

    const Symbol length_symbol = context::length_symbol(ctx);
    PropertyDescriptor old_len_desc_prop = GetOwnProperty(ctx, length_symbol);
    DataDescriptor* const old_len_desc = old_len_desc_prop.AsDataDescriptor();
    const JSVal& len_value = old_len_desc->value();
    assert(len_value.IsUInt32());

    if (name == length_symbol) {
      if (desc.IsDataDescriptor()) {
        const DataDescriptor* const data = desc.AsDataDescriptor();
        if (data->IsValueAbsent()) {
          // changing attribute [[Writable]] or TypeError.
          // [[Value]] not changed.
          return JSObject::DefineOwnProperty(ctx, name, desc, th, e);
        }
        DataDescriptor new_len_desc(*data);
        const double new_len_double =
            new_len_desc.value().ToNumber(ctx, ERROR_WITH(e, false));
        // length must be uint32_t
        const uint32_t new_len = core::DoubleToUInt32(new_len_double);
        if (new_len != new_len_double) {
          e->Report(Error::Range, "invalid array length");
          return false;
        }
        new_len_desc.set_value(JSVal::UInt32(new_len));
        uint32_t old_len = len_value.uint32();
        if (new_len >= old_len) {
          return JSObject::DefineOwnProperty(ctx, length_symbol,
                                             new_len_desc, th, e);
        }
        if (!old_len_desc->IsWritable()) {
          REJECT("\"length\" not writable");
        }
        const bool new_writable =
            new_len_desc.IsWritableAbsent() || new_len_desc.IsWritable();
        new_len_desc.set_writable(true);
        const bool succeeded = JSObject::DefineOwnProperty(ctx, length_symbol,
                                                           new_len_desc, th, e);
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
                  JSArray::DeleteWithIndex(ctx, old_len,
                                           false, ERROR_WITH(e, false));
              if (!delete_succeeded) {
                new_len_desc.set_value(JSVal::UInt32(old_len + 1));
                if (!new_writable) {
                  new_len_desc.set_writable(false);
                }
                JSObject::DefineOwnProperty(ctx, length_symbol,
                                            new_len_desc,
                                            false, ERROR_WITH(e, false));
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
              uint32_t index;
              if (core::ConvertToUInt32(context::GetSymbolString(ctx, *it),
                                        &index)) {
                ix.insert(index);
              }
            }
            for (std::set<uint32_t>::const_reverse_iterator it = ix.rbegin(),
                 last = ix.rend(); it != last; --it) {
              if (*it < new_len) {
                break;
              }
              const bool delete_succeeded = DeleteWithIndex(ctx, *it, false, e);
              if (!delete_succeeded) {
                const uint32_t result_len = *it + 1;
                CompactionToLength(result_len);
                new_len_desc.set_value(JSVal::UInt32(result_len));
                if (!new_writable) {
                  new_len_desc.set_writable(false);
                }
                JSObject::DefineOwnProperty(ctx, length_symbol,
                                            new_len_desc, false,
                                            ERROR_WITH(e, false));
                REJECT("shrink array failed");
              }
            }
            CompactionToLength(new_len);
          }
        }
        if (!new_writable) {
          JSObject::DefineOwnProperty(ctx, length_symbol,
                                      DataDescriptor(
                                          PropertyDescriptor::WRITABLE |
                                          PropertyDescriptor::UNDEF_ENUMERABLE |
                                          PropertyDescriptor::UNDEF_CONFIGURABLE),
                                      false, e);
        }
        return true;
      } else {
        // length is not configurable
        // so length is not changed
        return JSObject::DefineOwnProperty(ctx, name, desc, th, e);
      }
    } else {
      return JSObject::DefineOwnProperty(ctx, name, desc, th, e);
    }
  }

  bool DefineOwnPropertyWithIndex(Context* ctx,
                                  uint32_t index,
                                  const PropertyDescriptor& desc,
                                  bool th,
                                  Error* e) {
    // array index
    PropertyDescriptor old_len_desc_prop =
        JSArray::GetOwnProperty(ctx, context::length_symbol(ctx));
    DataDescriptor* const old_len_desc = old_len_desc_prop.AsDataDescriptor();
    const uint32_t old_len =
        old_len_desc->value().ToUInt32(ctx, ERROR_WITH(e, false));
    if (index >= old_len && !old_len_desc->IsWritable()) {
      return false;
    }

    // define step
    const bool descriptor_is_default_property = detail::IsDefaultDescriptor(desc);
    if (descriptor_is_default_property &&
        (dense_ ||
         JSObject::GetOwnProperty(ctx, context::Intern(ctx, index)).IsEmpty())) {
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
          map_ = new(GC)Map();
        }
        (*map_)[index] = target;
      }
    } else {
      const bool succeeded =
          JSObject::DefineOwnProperty(ctx,
                                      context::Intern(ctx, index),
                                      desc, false, ERROR_WITH(e, false));
      if (succeeded) {
        dense_ = false;
        if (detail::kMaxVectorSize > index) {
          if (vector_.size() > index) {
            vector_[index] = JSEmpty;
          }
        } else {
          if (map_) {
            const Map::iterator it = map_->find(index);
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
      old_len_desc->set_value(JSVal::UInt32(index+1));
      JSObject::DefineOwnProperty(ctx,
                                  context::length_symbol(ctx),
                                  *old_len_desc,
                                  false, e);
    }
    return true;
  }

#undef REJECT

  bool Delete(Context* ctx, Symbol name, bool th, Error* e) {
    uint32_t index;
    if (core::ConvertToUInt32(context::GetSymbolString(ctx, name), &index)) {
      return JSArray::DeleteWithIndex(ctx, index, th, e);
    }
    return JSObject::Delete(ctx, name, th, e);
  }

  bool DeleteWithIndex(Context* ctx, uint32_t index, bool th, Error* e) {
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
        const Map::iterator it = map_->find(index);
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
    if (const SymbolChecker check = SymbolChecker(ctx, index)) {
      return JSObject::Delete(ctx, check.symbol(), th, e);
    } else {
      return true;
    }
  }

  void GetOwnPropertyNames(Context* ctx,
                           std::vector<Symbol>* vec,
                           EnumerationMode mode) const {
    uint32_t index = 0;
    for (JSVals::const_iterator it = vector_.begin(),
         last = vector_.end(); it != last; ++it, ++index) {
      if (!it->IsEmpty()) {
        const Symbol sym = context::Intern(ctx, index);
        if (std::find(vec->begin(), vec->end(), sym) == vec->end()) {
          vec->push_back(sym);
        }
      }
    }
    if (map_) {
      for (Map::const_iterator it = map_->begin(),
           last = map_->end(); it != last; ++it) {
        if (!it->second.IsEmpty()) {
          const Symbol sym = context::Intern(ctx, it->first);
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
    const Class& cls = context::Cls(ctx, "Array");
    ary->set_class_name(cls.name);
    ary->set_prototype(cls.prototype);
    return ary;
  }

  static JSArray* New(Context* ctx, uint32_t n) {
    JSArray* const ary = new JSArray(ctx, n);
    const Class& cls = context::Cls(ctx, "Array");
    ary->set_class_name(cls.name);
    ary->set_prototype(cls.prototype);
    return ary;
  }

  static JSArray* NewPlain(Context* ctx) {
    return new JSArray(ctx, 0);
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
      map_ = new(GC)Map();
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
  Map* map_;
  bool dense_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARRAY_H_
