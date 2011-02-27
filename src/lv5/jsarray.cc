#include <limits>
#include <vector>
#include <set>
#include <algorithm>
#include <tr1/array>
#include <cstdlib>
#include "chars.h"
#include "conversions.h"
#include "ustringpiece.h"
#include "lv5/jsarray.h"
#include "lv5/jsobject.h"
#include "lv5/property.h"
#include "lv5/jsstring.h"
#include "lv5/context_utils.h"
#include "lv5/context.h"
#include "lv5/class.h"

namespace iv {
namespace lv5 {

JSArray::JSArray(Context* ctx, std::size_t len)
  : JSObject(),
    vector_((len < detail::kMaxVectorSize) ? len : 4, JSEmpty),
    map_(NULL),
    dense_(true),
    length_(len) {
  JSObject::DefineOwnProperty(ctx, ctx->length_symbol(),
                              DataDescriptor(len,
                                             PropertyDescriptor::WRITABLE),
                                             false, ctx->error());
}

PropertyDescriptor JSArray::GetOwnProperty(Context* ctx, Symbol name) const {
  uint32_t index;
  if (core::ConvertToUInt32(context::GetSymbolString(ctx, name), &index)) {
    return JSArray::GetOwnPropertyWithIndex(ctx, index);
  }
  return JSObject::GetOwnProperty(ctx, name);
}

PropertyDescriptor JSArray::GetOwnPropertyWithIndex(Context* ctx,
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
  const SymbolChecker check(ctx, index);
  if (check.Found()) {
    return JSObject::GetOwnProperty(ctx, check.symbol());
  } else {
    // property not defined
    return JSUndefined;
  }
}

#define REJECT(str)\
  do {\
    if (th) {\
      res->Report(Error::Type, str);\
    }\
    return false;\
  } while (0)

bool JSArray::DefineOwnProperty(Context* ctx,
                                Symbol name,
                                const PropertyDescriptor& desc,
                                bool th,
                                Error* res) {
  uint32_t index;
  if (core::ConvertToUInt32(context::GetSymbolString(ctx, name), &index)) {
    return JSArray::DefineOwnPropertyWithIndex(ctx, index, desc, th, res);
  }

  const Symbol length_symbol = ctx->length_symbol();
  PropertyDescriptor old_len_desc_prop = GetOwnProperty(ctx, length_symbol);
  DataDescriptor* const old_len_desc = old_len_desc_prop.AsDataDescriptor();
  const JSVal& len_value = old_len_desc->value();
  assert(len_value.IsNumber());

  if (name == length_symbol) {
    if (desc.IsDataDescriptor()) {
      const DataDescriptor* const data = desc.AsDataDescriptor();
      if (data->IsValueAbsent()) {
        // changing attribute [[Writable]] or TypeError.
        // [[Value]] not changed.
        return JSObject::DefineOwnProperty(ctx, name, desc, th, res);
      }
      DataDescriptor new_len_desc(*data);
      const double new_len_double = new_len_desc.value().ToNumber(ctx, res);
      if (*res) {
        return false;
      }
      // length must be uint32_t
      const uint32_t new_len = core::DoubleToUInt32(new_len_double);
      if (new_len != new_len_double) {
        res->Report(Error::Range, "invalid array length");
        return false;
      }
      new_len_desc.set_value(new_len);
      uint32_t old_len = core::DoubleToUInt32(len_value.number());
      if (*res) {
        return false;
      }
      if (new_len >= old_len) {
        return JSObject::DefineOwnProperty(ctx, length_symbol,
                                           new_len_desc, th, res);
      }
      if (!old_len_desc->IsWritable()) {
        REJECT("\"length\" not writable");
      }
      const bool new_writable =
          new_len_desc.IsWritableAbsent() || new_len_desc.IsWritable();
      new_len_desc.set_writable(true);
      const bool succeeded = JSObject::DefineOwnProperty(ctx, length_symbol,
                                                         new_len_desc, th, res);
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
                JSArray::DeleteWithIndex(ctx, old_len, false, res);
            if (*res) {
              return false;
            }
            if (!delete_succeeded) {
              new_len_desc.set_value(old_len + 1);
              if (!new_writable) {
                new_len_desc.set_writable(false);
              }
              JSObject::DefineOwnProperty(ctx, length_symbol,
                                          new_len_desc, false, res);
              if (*res) {
                return false;
              }
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
            const bool delete_succeeded = DeleteWithIndex(ctx, *it, false, res);
            if (!delete_succeeded) {
              const uint32_t result_len = *it + 1;
              CompactionToLength(result_len);
              new_len_desc.set_value(result_len);
              if (!new_writable) {
                new_len_desc.set_writable(false);
              }
              JSObject::DefineOwnProperty(ctx, length_symbol,
                                          new_len_desc, false, res);
              if (*res) {
                return false;
              }
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
                                    false, res);
      }
      return true;
    } else {
      // length is not configurable
      // so length is not changed
      return JSObject::DefineOwnProperty(ctx, name, desc, th, res);
    }
  } else {
    return JSObject::DefineOwnProperty(ctx, name, desc, th, res);
  }
}

bool JSArray::DefineOwnPropertyWithIndex(Context* ctx,
                                         uint32_t index,
                                         const PropertyDescriptor& desc,
                                         bool th,
                                         Error* res) {
  // array index
  PropertyDescriptor old_len_desc_prop =
      JSArray::GetOwnProperty(ctx, ctx->length_symbol());
  DataDescriptor* const old_len_desc = old_len_desc_prop.AsDataDescriptor();
  const double old_len = old_len_desc->value().ToNumber(ctx, res);
  if (*res) {
    return false;
  }
  if (index >= old_len && !old_len_desc->IsWritable()) {
    return false;
  }

  // define step
  const bool descriptor_is_default_property = IsDefaultDescriptor(desc);
  if (descriptor_is_default_property &&
      (dense_ ||
       JSObject::GetOwnProperty(ctx, ctx->InternIndex(index)).IsEmpty())) {
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
    const bool succeeded = JSObject::DefineOwnProperty(ctx,
                                                       ctx->InternIndex(index),
                                                       desc, false, res);
    if (*res) {
      return false;
    }

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
    old_len_desc->set_value(index+1);
    JSObject::DefineOwnProperty(ctx, ctx->length_symbol(),
                                *old_len_desc, false, res);
  }
  return true;
}

#undef REJECT


bool JSArray::Delete(Context* ctx, Symbol name, bool th, Error* res) {
  uint32_t index;
  if (core::ConvertToUInt32(context::GetSymbolString(ctx, name), &index)) {
    return JSArray::DeleteWithIndex(ctx, index, th, res);
  }
  return JSObject::Delete(ctx, name, th, res);
}

bool JSArray::DeleteWithIndex(Context* ctx,
                              uint32_t index, bool th, Error* res) {
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
  const SymbolChecker check(ctx, index);
  if (check.Found()) {
    return JSObject::Delete(ctx, check.symbol(), th, res);
  } else {
    return true;
  }
}

void JSArray::GetOwnPropertyNames(Context* ctx,
                                  std::vector<Symbol>* vec,
                                  EnumerationMode mode) const {
  using std::find;
  if (vec->empty()) {
    uint32_t index = 0;
    for (JSVals::const_iterator it = vector_.begin(),
         last = vector_.end(); it != last; ++it, ++index) {
      if (!it->IsEmpty()) {
        vec->push_back(ctx->InternIndex(index));
      }
    }
    if (map_) {
      for (Map::const_iterator it = map_->begin(),
           last = map_->end(); it != last; ++it) {
        if (!it->second.IsEmpty()) {
          vec->push_back(ctx->InternIndex(it->first));
        }
      }
    }
  } else {
    uint32_t index = 0;
    for (JSVals::const_iterator it = vector_.begin(),
         last = vector_.end(); it != last; ++it, ++index) {
      if (!it->IsEmpty()) {
        const Symbol sym = ctx->InternIndex(index);
        if (find(vec->begin(), vec->end(), sym) == vec->end()) {
          vec->push_back(sym);
        }
      }
    }
    if (map_) {
      for (Map::const_iterator it = map_->begin(),
           last = map_->end(); it != last; ++it) {
        if (!it->second.IsEmpty()) {
          const Symbol sym = ctx->InternIndex(it->first);
          if (find(vec->begin(), vec->end(), sym) == vec->end()) {
            vec->push_back(sym);
          }
        }
      }
    }
  }
  JSObject::GetOwnPropertyNames(ctx, vec, mode);
}

JSArray* JSArray::New(Context* ctx) {
  JSArray* const ary = new JSArray(ctx, 0);
  const Class& cls = ctx->Cls("Array");
  ary->set_class_name(cls.name);
  ary->set_prototype(cls.prototype);
  return ary;
}

JSArray* JSArray::New(Context* ctx, std::size_t n) {
  JSArray* const ary = new JSArray(ctx, n);
  const Class& cls = ctx->Cls("Array");
  ary->set_class_name(cls.name);
  ary->set_prototype(cls.prototype);
  return ary;
}

JSArray* JSArray::NewPlain(Context* ctx) {
  return new JSArray(ctx, 0);
}

bool JSArray::IsDefaultDescriptor(const PropertyDescriptor& desc) {
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

void JSArray::CompactionToLength(uint32_t length) {
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

} }  // namespace iv::lv5
