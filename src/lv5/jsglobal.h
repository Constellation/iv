#ifndef _IV_LV5_JSGLOBAL_H_
#define _IV_LV5_JSGLOBAL_H_
#include "notfound.h"
#include "lv5/error_check.h"
#include "lv5/map.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
#include "lv5/class.h"
#include "lv5/arguments.h"
#include "lv5/object_utils.h"
namespace iv {
namespace lv5 {

class JSGlobal : public JSObject {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "global",
      Class::global
    };
    return &cls;
  }

  explicit JSGlobal(Context* ctx)
    : slots_(),
      map_(Map::NewUniqueMap(ctx)) {
    assert(map_->GetSlotsSize() == 0);
  }

  inline JSVal Get(Context* ctx, Symbol name, Error* e);

  inline PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;

  inline bool Delete(Context* ctx, Symbol name, bool th, Error* e);

  inline bool DefineOwnProperty(Context* ctx,
                               Symbol name,
                               const PropertyDescriptor& desc,
                               bool th,
                               Error* e);

  inline JSVal GetFromSlot(Context* ctx,
                           std::size_t offset, Error* e);

  inline void PutToSlot(Context* ctx,
                        std::size_t offset,
                        const JSVal& val,
                        bool th,
                        Error* e);

  inline void GetPropertyNames(Context* ctx,
                               std::vector<Symbol>* vec, EnumerationMode mode) const;

  inline void GetOwnPropertyNames(Context* ctx,
                                  std::vector<Symbol>* vec,
                                  EnumerationMode mode) const;

  inline std::size_t GetOwnPropertySlot(Context* ctx, Symbol name) const;

  inline void CreateProperty(Context* ctx,
                             Symbol name, const PropertyDescriptor& desc);

  const PropertyDescriptor& GetSlot(std::size_t n) const {
    return slots_[n];
  }

  PropertyDescriptor& GetSlot(std::size_t n) {
    return slots_[n];
  }

  Map* map() const {
    return map_;
  }

 private:
  GCVector<PropertyDescriptor>::type slots_;
  Map* map_;
};


JSVal JSGlobal::Get(Context* ctx,
                    Symbol name, Error* e) {
  const PropertyDescriptor desc = GetProperty(ctx, name);
  if (desc.IsEmpty()) {
    return JSUndefined;
  }
  if (desc.IsDataDescriptor()) {
    return desc.AsDataDescriptor()->value();
  } else {
    assert(desc.IsAccessorDescriptor());
    JSObject* const getter = desc.AsAccessorDescriptor()->get();
    if (getter) {
      ScopedArguments a(ctx, 0, IV_LV5_ERROR(e));
      return getter->AsCallable()->Call(&a, this, e);
    } else {
      return JSUndefined;
    }
  }
}

std::size_t JSGlobal::GetOwnPropertySlot(Context* ctx, Symbol name) const {
  return map_->Get(ctx, name);
}

PropertyDescriptor JSGlobal::GetOwnProperty(Context* ctx, Symbol name) const {
  const std::size_t offset = GetOwnPropertySlot(ctx, name);
  if (offset != core::kNotFound) {
    // found
    return slots_[offset];
  } else {
    // not found
    return JSUndefined;
  }
}

bool JSGlobal::DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th,
                                 Error* e) {
  // section 8.12.9 [[DefineOwnProperty]]
  const std::size_t offset = map_->Get(ctx, name);
  if (offset != core::kNotFound) {
    // found
    const PropertyDescriptor current = slots_[offset];
    bool returned = false;
    if (IsDefineOwnPropertyAccepted(current, desc, th, &returned, e)) {
      slots_[offset] = PropertyDescriptor::Merge(desc, current);
    }
    return returned;
  } else {
    // not found
    if (!IsExtensible()) {
      if (th) {
        e->Report(Error::Type, "object not extensible");\
      }
      return false;
    } else {
      // newly create property => map transition
      CreateProperty(ctx, name, PropertyDescriptor::SetDefault(desc));
      return true;
    }
  }
}


JSVal JSGlobal::GetFromSlot(Context* ctx,
                            std::size_t offset, Error* e) {
  const PropertyDescriptor& desc = slots_[offset];
  if (desc.IsDataDescriptor()) {
    return desc.AsDataDescriptor()->value();
  } else {
    assert(desc.IsAccessorDescriptor());
    JSObject* const getter = desc.AsAccessorDescriptor()->get();
    if (getter) {
      ScopedArguments a(ctx, 0, IV_LV5_ERROR(e));
      return getter->AsCallable()->Call(&a, this, e);
    } else {
      return JSUndefined;
    }
  }
}

void JSGlobal::PutToSlot(Context* ctx,
                         std::size_t offset, const JSVal& val,
                         bool th, Error* e) {
  // not empty is already checked
  const PropertyDescriptor current = GetSlot(offset);
  // can put check
  if ((current.IsAccessorDescriptor() && !current.AsAccessorDescriptor()->set()) ||
      (current.IsDataDescriptor() && !current.AsDataDescriptor()->IsWritable())) {
    if (th) {
      e->Report(Error::Type, "put failed");
    }
    return;
  }
  assert(!current.IsEmpty());
  if (current.IsDataDescriptor()) {
    const DataDescriptor desc(
        val,
        PropertyDescriptor::UNDEF_ENUMERABLE |
        PropertyDescriptor::UNDEF_CONFIGURABLE |
        PropertyDescriptor::UNDEF_WRITABLE);
    bool returned = false;
    if (IsDefineOwnPropertyAccepted(current, desc, th, &returned, e)) {
      slots_[offset] = PropertyDescriptor::Merge(desc, current);
    }
  } else {
    const AccessorDescriptor* const accs = current.AsAccessorDescriptor();
    assert(accs->set());
    ScopedArguments args(ctx, 1, IV_LV5_ERROR_VOID(e));
    args[0] = val;
    accs->set()->AsCallable()->Call(&args, this, e);
  }
}

void JSGlobal::CreateProperty(Context* ctx,
                              Symbol name, const PropertyDescriptor& desc) {
  // add property transition
  // searching already created maps and if this is available, move to this
  std::size_t offset;
  map_ = map_->AddPropertyTransition(ctx, name, &offset);
  slots_.resize(map_->GetSlotsSize(), JSUndefined);
  slots_[offset] = desc;  // set newly created property
}

bool JSGlobal::Delete(Context* ctx, Symbol name, bool th, Error* e) {
  const std::size_t offset = map_->Get(ctx, name);
  if (offset == core::kNotFound) {
    return true;  // not found
  }
  if (!slots_[offset].IsConfigurable()) {
    if (th) {
      e->Report(Error::Type, "delete failed");
    }
    return false;
  }

  // delete property transition
  // if previous map is avaiable shape, move to this.
  // and if that is not avaiable, create new map and move to it.
  // newly created slots size is always smaller than before
  map_ = map_->DeletePropertyTransition(ctx, name);
  slots_[offset] = JSUndefined;
  return true;
}

void JSGlobal::GetOwnPropertyNames(Context* ctx,
                                   std::vector<Symbol>* vec,
                                   EnumerationMode mode) const {
  map_->GetOwnPropertyNames(this, ctx, vec, mode);
}


void Map::GetOwnPropertyNames(const JSGlobal* obj,
                              Context* ctx,
                              std::vector<Symbol>* vec,
                              JSObject::EnumerationMode mode) {
  if (!HasTable()) {
    AllocateTable(this);
  }
  for (TargetTable::const_iterator it = table_->begin(),
       last = table_->end(); it != last; ++it) {
    if ((mode == JSObject::kIncludeNotEnumerable ||
         obj->GetSlot(it->second).IsEnumerable()) &&
        (std::find(vec->begin(), vec->end(), it->first) == vec->end())) {
      vec->push_back(it->first);
    }
  }
}


} }  // namespace iv::lv5
#endif  // _IV_LV5_JSGLOBAL_H_
