#ifndef IV_LV5_JSARRAY_H_
#define IV_LV5_JSARRAY_H_
#include <cstdlib>
#include <limits>
#include <vector>
#include <set>
#include <algorithm>
#include <iv/conversions.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/property.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/map.h>
#include <iv/lv5/slot.h>
#include <iv/lv5/class.h>
#include <iv/lv5/storage.h>
#include <iv/lv5/adapter/select1st.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {
namespace jsarray_detail {

static bool IsAbsentDescriptor(const PropertyDescriptor& desc) {
  if (!desc.IsEnumerable() && !desc.IsEnumerableAbsent()) {
    // explicitly not enumerable
    return false;
  }
  if (!desc.IsConfigurable() && !desc.IsConfigurableAbsent()) {
    // explicitly not configurable
    return false;
  }
  if (desc.IsAccessor()) {
    return false;
  }
  if (desc.IsData()) {
    const DataDescriptor* const data = desc.AsDataDescriptor();
    return data->IsWritable() || data->IsWritableAbsent();
  }
  return true;
}

template<typename T = void>
class JSArrayConstants {
 public:
  static const uint32_t kMaxVectorSize;
};

template<typename T>
const uint32_t JSArrayConstants<T>::kMaxVectorSize = 10000;

}  // namespace iv::lv5::jsarray_detail

class Context;
class JSVector;

class JSArray : public JSObject, public jsarray_detail::JSArrayConstants<> {
 public:
  IV_LV5_DEFINE_JSCLASS(Array)

  friend class railgun::VM;
  friend class breaker::Compiler;
  friend class JSVector;
  typedef GCHashMap<uint32_t, JSVal>::type SparseArray;
  typedef Storage<JSVal> JSValVector;

  uint32_t GetLength() const {
    uint32_t length = 0;  // makes compiler happy
    length_.value().GetUInt32(&length);
    return length;
  }

  static JSArray* New(Context* ctx) {
    JSArray* const ary = new JSArray(ctx, 0u);
    ary->set_cls(GetClass());
    ary->set_prototype(ctx->global_data()->array_prototype());
    return ary;
  }

  static JSArray* New(Context* ctx, uint32_t n) {
    JSArray* const ary = new JSArray(ctx, n);
    ary->set_cls(GetClass());
    ary->set_prototype(ctx->global_data()->array_prototype());
    return ary;
  }

  static JSArray* New(Context* ctx, JSArray* array) {
    JSArray* const ary = new JSArray(ctx, array);
    ary->set_cls(GetClass());
    ary->set_prototype(ctx->global_data()->array_prototype());
    return ary;
  }

  template<typename Iter>
  static JSArray* New(Context* ctx, Iter it, Iter last) {
    const uint32_t length = std::distance(it, last);
    JSArray* ary = JSArray::New(ctx, length);
    const uint32_t min = std::min(length, JSArray::kMaxVectorSize);
    const Iter mid = it + min;
    ary->SetToVector(0, it, mid);
    if (mid != last) {
      uint32_t current = JSArray::kMaxVectorSize;
      ary->ReserveMap(length);
      ary->SetToMap(current, mid, last);
    }
    return ary;
  }

  static JSArray* NewPlain(Context* ctx) {
    return new JSArray(ctx, 0u);
  }

  static JSArray* NewPlain(Context* ctx, Map* map) {
    return new JSArray(ctx, map, 0u);
  }

  virtual bool GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
    if (symbol::IsArrayIndexSymbol(name)) {
      slot->MakeUnCacheable();
      const uint32_t index = symbol::GetIndexFromSymbol(name);
      if (!dense_) {
        Slot slot2;
        if (JSObject::GetOwnPropertySlot(ctx, name, &slot2)) {
          *slot = slot2;
          return true;
        }
      }
      // this target included in vector (if dense array)
      if (vector_.size() > index) {
        const JSVal val = vector_[index];
        if (!val.IsEmpty()) {
          // current is target
          slot->set(val, Attributes::CreateData(ATTR::W | ATTR::E | ATTR::C), this);
          return true;
        }
        return false;
      } else if (kMaxVectorSize > index) {
        return false;
      } else {
        // target is index and included in map
        if (map_) {
          const SparseArray::const_iterator it = map_->find(index);
          if (it != map_->end()) {
            // target is found
            slot->set(it->second, Attributes::CreateData(ATTR::W | ATTR::E | ATTR::C), this);
            return true;
          }
        }
      }
      return false;
    }
    if (name == symbol::length()) {
      slot->set(length_.value(), length_.attributes(), this);
      return true;
    }
    return JSObject::GetOwnPropertySlot(ctx, name, slot);
  }

  virtual bool DefineOwnPropertySlot(Context* ctx,
                                     Symbol name,
                                     const PropertyDescriptor& desc,
                                     Slot* slot,
                                     bool th,
                                     Error* e) {
    if (symbol::IsArrayIndexSymbol(name)) {
      // section 15.4.5.1 step 4
      return DefineArrayIndexProperty(ctx, name, desc, slot, th, e);
    } else if (name == symbol::length()) {
      // section 15.4.5.1 step 3
      return DefineLengthProperty(ctx, desc, th, e);
    } else {
      // section 15.4.5.1 step 5
      return JSObject::DefineOwnPropertySlot(ctx, name, desc, slot, th, e);
    }
  }

  virtual bool Delete(Context* ctx, Symbol name, bool th, Error* e) {
    if (symbol::IsArrayIndexSymbol(name)) {
      const uint32_t index = symbol::GetIndexFromSymbol(name);
      if (kMaxVectorSize > index) {
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
      return JSObject::DeleteDirect(ctx, name, th, e);
    }
    if (symbol::length() == name) {
      if (th) {
        e->Report(Error::Type, "delete failed");
      }
      return false;
    }
    return JSObject::DeleteDirect(ctx, name, th, e);
  }

  virtual void GetOwnPropertyNames(Context* ctx,
                                   PropertyNamesCollector* collector,
                                   EnumerationMode mode) const {
    if (length_.attributes().IsEnumerable() || (mode == INCLUDE_NOT_ENUMERABLE)) {
      collector->Add(symbol::length(), 0);
    }

    uint32_t index = 0;
    for (JSValVector::const_iterator it = vector_.begin(),
         last = vector_.end(); it != last; ++it, ++index) {
      if (!it->IsEmpty()) {
        collector->Add(index);
      }
    }
    if (map_) {
      for (SparseArray::const_iterator it = map_->begin(),
           last = map_->end(); it != last; ++it) {
        if (!it->second.IsEmpty()) {
          collector->Add(it->first);
        }
      }
    }
    JSObject::GetOwnPropertyNames(ctx, collector, mode);
  }

  virtual void MarkChildren(radio::Core* core) {
    JSObject::MarkChildren(core);
    std::for_each(vector_.begin(), vector_.end(), radio::Core::Marker(core));
    if (map_) {
      for (SparseArray::const_iterator it = map_->begin(),
           last = map_->end(); it != last; ++it) {
        core->MarkValue(it->second);
      }
    }
  }

  // Array optimizations
  inline bool CanGetIndexDirect(uint32_t index) const {
    return dense_ && vector_.size() > index && !vector_[index].IsEmpty();
  }

  inline JSVal GetIndexDirect(uint32_t index) const {
    assert(CanGetIndexDirect(index));
    return vector_[index];
  }

  inline bool CanSetIndexDirect(uint32_t index) const {
    return dense_ && vector_.size() > index;
  }

  inline void SetIndexDirect(uint32_t index, JSVal val) {
    assert(CanSetIndexDirect(index));
    vector_[index] = val;
  }

  inline void Reserve(uint32_t len) {
    vector_.reserve((len > kMaxVectorSize) ? kMaxVectorSize : len);
  }

  static std::size_t VectorOffset() {
    return IV_OFFSETOF(JSArray, vector_);
  }

  static std::size_t LengthOffset() {
    return IV_OFFSETOF(JSArray, length_);
  }

  static std::size_t DenseOffset() {
    return IV_OFFSETOF(JSArray, dense_);
  }

 private:
  JSArray(Context* ctx, uint32_t len)
    : JSObject(ctx->global_data()->array_map()),
      vector_((len <= kMaxVectorSize) ? len : 4, JSEmpty),
      map_(NULL),
      length_(len, Attributes::CreateData(ATTR::WRITABLE)),
      dense_(true) {
  }

  JSArray(Context* ctx, Map* map, uint32_t len)
    : JSObject(map),
      vector_((len <= kMaxVectorSize) ? len : 4, JSEmpty),
      map_(NULL),
      length_(len, Attributes::CreateData(ATTR::WRITABLE)),
      dense_(true) {
  }

  JSArray(Context* ctx, JSArray* array)
    : JSObject(Map::NewFromPoint(ctx, array->map())),
      vector_(array->vector_),
      map_(array->map_ ? new(GC)SparseArray(*array->map_) : NULL),
      length_(array->length_),
      dense_(array->dense_) {
  }

#define REJECT(str)\
  do {\
    if (th) {\
      e->Report(Error::Type, str);\
    }\
    return false;\
  } while (0)

  // section 15.4.5.1 step 4
  bool DefineArrayIndexProperty(Context* ctx,
                                Symbol name,
                                const PropertyDescriptor& desc,
                                Slot* slot,
                                bool th, Error* e) {
    // 15.4.5.1 step 4-a
    const uint32_t index = symbol::GetIndexFromSymbol(name);
    const uint32_t old_len = GetLength();
    // 15.4.5.1 step 4-b
    if (index >= old_len && !length_.attributes().IsWritable()) {
      REJECT("adding an element to the array "
             "which length is not writable is rejected");
    }

    // dense array optimization code
    const bool is_default_descriptor = desc.IsDefault();
    if (is_default_descriptor ||
        (index < old_len && jsarray_detail::IsAbsentDescriptor(desc))) {
      bool use_dense_path = dense_;
      if (!use_dense_path) {
        if (!slot->IsUsed()) {
          GetOwnPropertySlot(ctx, name, slot);
        }
        use_dense_path = slot->IsNotFound() || slot->base() != this;
      }
      if (use_dense_path) {
        if (kMaxVectorSize > index) {
          if (vector_.size() > index) {
            if (vector_[index].IsEmpty()) {
              if (is_default_descriptor) {
                if (desc.AsDataDescriptor()->IsValueAbsent()) {
                  vector_[index] = JSUndefined;
                } else {
                  vector_[index] = desc.AsDataDescriptor()->value();
                }
                return FixUpLength(old_len, index);
              }
              // through like
              //
              // var ary = new Array(10);
              // Object.defineProperty(ary, '0', { });
              //
            } else {
              if (desc.IsData() &&
                  !desc.AsDataDescriptor()->IsValueAbsent()) {
                vector_[index] = desc.AsDataDescriptor()->value();
              }
              return FixUpLength(old_len, index);
            }
          } else {
            if (is_default_descriptor) {
              vector_.resize(index + 1, JSEmpty);
              if (desc.AsDataDescriptor()->IsValueAbsent()) {
                vector_[index] = JSUndefined;
              } else {
                vector_[index] = desc.AsDataDescriptor()->value();
              }
              return FixUpLength(old_len, index);
            }
          }
        } else {
          if (!map_) {
            map_ = new(GC)SparseArray;
            if (is_default_descriptor) {
              if (desc.AsDataDescriptor()->IsValueAbsent()) {
                (*map_)[index] = JSUndefined;
              } else {
                (*map_)[index] = desc.AsDataDescriptor()->value();
              }
              return FixUpLength(old_len, index);
            }
          } else {
            SparseArray::iterator it = map_->find(index);
            if (it != map_->end()) {
              if (desc.IsData() &&
                  !desc.AsDataDescriptor()->IsValueAbsent()) {
                (*map_)[index] = desc.AsDataDescriptor()->value();
              }
              return FixUpLength(old_len, index);
            } else {
              if (is_default_descriptor) {
                if (desc.AsDataDescriptor()->IsValueAbsent()) {
                  (*map_)[index] = JSUndefined;
                } else {
                  (*map_)[index] = desc.AsDataDescriptor()->value();
                }
                return FixUpLength(old_len, index);
              }
            }
          }
        }
      }
    }
    // 15.4.5.1 step 4-c
    const bool succeeded =
        JSObject::DefineOwnPropertySlot(
            ctx, name, desc, slot, false, IV_LV5_ERROR_WITH(e, false));
    // 15.4.5.1 step 4-d
    if (!succeeded) {
      REJECT("define own property failed");
    }

    // move state from dense array to not dense array
    dense_ = false;
    if (kMaxVectorSize > index) {
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
    // 15.4.5.1 step 4-e, 4-f
    return FixUpLength(old_len, index);
  }

  // section 15.4.5.1 step 3
  bool DefineLengthProperty(Context* ctx,
                            const PropertyDescriptor& desc,
                            bool th, Error* e) {
    if (!desc.IsData()) {
      // length is not configurable
      // so length is not changed
      bool returned = false;
      length_.IsDefineOwnPropertyAccepted(desc, th, &returned, e);
      return returned;
    }

    const DataDescriptor* const data = desc.AsDataDescriptor();
    if (data->IsValueAbsent()) {
      // GenericDescriptor
      // changing attribute [[Writable]] or TypeError.
      // [[Value]] not changed.
      //
      // length value is always not empty, so use
      // GetDefineOwnPropertyResult
      bool returned = false;
      if (length_.IsDefineOwnPropertyAccepted(desc, th, &returned, e)) {
        length_.Merge(ctx, desc);
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
    DataDescriptor new_len_desc = *data;
    new_len_desc.set_value(JSVal::UInt32(new_len));
    uint32_t old_len = GetLength();
    if (new_len >= old_len) {
      bool returned = false;
      if (length_.IsDefineOwnPropertyAccepted(new_len_desc, th, &returned, e)) {
        length_.Merge(ctx, new_len_desc);
      }
      return returned;
    }
    if (!length_.attributes().IsWritable()) {
      REJECT("\"length\" not writable");
    }
    const bool new_writable =
        new_len_desc.IsWritableAbsent() || new_len_desc.IsWritable();
    // 15.4.5.1 step 3-i
    if (!new_writable) {
      new_len_desc.set_writable(true);
    }
    bool succeeded = false;
    if (length_.IsDefineOwnPropertyAccepted(new_len_desc, th, &succeeded, e)) {
      length_.Merge(ctx, new_len_desc);
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
            if (length_.IsDefineOwnPropertyAccepted(new_len_desc, false, &wasted, e)) {
              length_.Merge(ctx, new_len_desc);
            }
            IV_LV5_ERROR_GUARD_WITH(e, false);
            REJECT("shrink array failed");
          }
        }
      } else {
        PropertyNamesCollector collector;
        JSObject::GetOwnPropertyNames(ctx, &collector, INCLUDE_NOT_ENUMERABLE);
        std::set<uint32_t> ix;
        for (PropertyNamesCollector::Names::const_iterator
             it = collector.names().begin(),
             last = collector.names().end();
             it != last; ++it) {
          if (symbol::IsArrayIndexSymbol(*it)) {
            ix.insert(symbol::GetIndexFromSymbol(*it));
          } else {
            break;
          }
        }
        for (std::set<uint32_t>::const_reverse_iterator it = ix.rbegin(),
             last = ix.rend(); it != last; ++it) {
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
            if (length_.IsDefineOwnPropertyAccepted(new_len_desc, false, &wasted, e)) {
              length_.Merge(ctx, new_len_desc);
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
          ATTR::UNDEF_ENUMERABLE |
          ATTR::UNDEF_CONFIGURABLE);
      bool wasted = false;
      if (length_.IsDefineOwnPropertyAccepted(target, false, &wasted, e)) {
        length_.Merge(ctx, target);
      }
    }
    return true;
  }

#undef REJECT

  void CompactionToLength(uint32_t length) {
    if (length > kMaxVectorSize) {
      if (map_) {
        std::vector<uint32_t> copy(map_->size());
        std::transform(map_->begin(), map_->end(),
                       copy.begin(),
                       adapter::select1st<SparseArray::value_type>());
        for (std::vector<uint32_t>::const_iterator it = copy.begin(),
             last = copy.end(); it != last; ++it) {
          if (*it >= length) {
            map_->erase(*it);
          }
        }
        if (map_->empty()) {
          map_ = NULL;
        }
      }
    } else {
      if (map_) {
        map_ = NULL;
      }
      if (vector_.size() > length) {
        vector_.resize(length, JSEmpty);
      }
    }
  }

 public:
  // use VM only
  //   ReservedNew
  //   Reserve
  //   Set
  static JSArray* ReservedNew(Context* ctx, uint32_t len) {
    JSArray* ary = New(ctx, len);
    ary->ReserveMap(len);
    return ary;
  }

  void ReserveMap(uint32_t len) {
    if (len > kMaxVectorSize) {
      // alloc map
      map_ = new(GC)SparseArray();
    }
  }

  template<typename Iter>
  void SetToVector(uint32_t index, Iter it, Iter last) {
    assert(kMaxVectorSize > index);
    assert(vector_.size() >=
           (index + static_cast<std::size_t>(std::distance(it, last))));
    std::copy(it, last, vector_.begin() + index);
  }

  template<typename Iter>
  void SetToMap(uint32_t index, Iter it, Iter last) {
    assert(kMaxVectorSize <= index);
    assert(map_);
    for (; it != last; ++it, ++index) {
      if (!it->IsEmpty()) {
        (*map_)[index] = *it;
      }
    }
  }

 private:
  bool FixUpLength(uint32_t old_len, uint32_t index) {
    if (index >= old_len) {
      length_.set_value(JSVal::UInt32(index + 1));
    }
    return true;
  }

  JSValVector vector_;
  SparseArray* map_;
  StoredSlot length_;
  uint8_t dense_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSARRAY_H_
