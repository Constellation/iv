#include <cassert>
#include <algorithm>
#include "lv5/jsobject.h"
#include "lv5/context_utils.h"
#include "lv5/property.h"
#include "lv5/jsfunction.h"
#include "lv5/jsval.h"
#include "lv5/jsenv.h"
#include "lv5/context.h"
#include "lv5/class.h"
#include "lv5/object_utils.h"
#include "lv5/jsbooleanobject.h"
#include "lv5/jsnumberobject.h"
#include "lv5/map.h"
#include "lv5/slot.h"
#include "lv5/error_check.h"
namespace iv {
namespace lv5 {

JSObject::JSObject(Map* map)
  : cls_(NULL),
    prototype_(NULL),
    extensible_(true),
    map_(map),
    slots_() {
}

JSObject::JSObject(Map* map,
                   JSObject* proto,
                   Class* cls,
                   bool extensible)
  : cls_(cls),
    prototype_(proto),
    extensible_(extensible),
    map_(map),
    slots_() {
}

#define TRY(context, sym, arg, error)\
  do {\
    const JSVal method = Get(context, sym, error);\
    if (*error) {\
      return JSUndefined;\
    }\
    if (method.IsCallable()) {\
      const JSVal val = method.object()->AsCallable()->Call(&arg, this, error);\
      if (*error) {\
        return JSUndefined;\
      }\
      if (val.IsPrimitive() || val.IsNull() || val.IsUndefined()) {\
        return val;\
      }\
    }\
  } while (0)
JSVal JSObject::DefaultValue(Context* ctx,
                             Hint::Object hint, Error* e) {
  ScopedArguments args(ctx, 0, IV_LV5_ERROR(e));
  if (hint == Hint::STRING) {
    // hint is STRING
    TRY(ctx, symbol::toString(), args, e);
    TRY(ctx, symbol::valueOf(), args, e);
  } else {
    // section 8.12.8
    // hint is NUMBER or NONE
    TRY(ctx, symbol::valueOf(), args, e);
    TRY(ctx, symbol::toString(), args, e);
  }
  e->Report(Error::Type, "invalid default value");
  return JSUndefined;
}
#undef TRY

JSVal JSObject::Get(Context* ctx,
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

// not recursion
PropertyDescriptor JSObject::GetProperty(Context* ctx, Symbol name) const {
  Slot slot;
  if (GetPropertySlot(ctx, name, &slot)) {
    return slot.desc();
  }
  return JSUndefined;
}

bool JSObject::GetPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
  const JSObject* obj = this;
  do {
    if (obj->GetOwnPropertySlot(ctx, name, slot)) {
      assert(!slot->desc().IsEmpty());
      return true;
    }
    obj = obj->prototype();
  } while (obj);
  return false;
}

PropertyDescriptor JSObject::GetOwnProperty(Context* ctx, Symbol name) const {
  Slot slot;
  if (GetOwnPropertySlot(ctx, name, &slot)) {
    assert(!slot.desc().IsEmpty());
    return slot.desc();
  } else {
    return JSUndefined;
  }
}

bool JSObject::CanPut(Context* ctx, Symbol name) const {
  const PropertyDescriptor desc = GetOwnProperty(ctx, name);
  if (!desc.IsEmpty()) {
    if (desc.IsAccessorDescriptor()) {
      return desc.AsAccessorDescriptor()->set();
    } else {
      assert(desc.IsDataDescriptor());
      return desc.AsDataDescriptor()->IsWritable();
    }
  }
  if (!prototype_) {
    return extensible_;
  }
  const PropertyDescriptor inherited = prototype_->GetProperty(ctx, name);
  if (inherited.IsEmpty()) {
    return extensible_;
  } else {
    if (inherited.IsAccessorDescriptor()) {
      return inherited.AsAccessorDescriptor()->set();
    } else {
      assert(inherited.IsDataDescriptor());
      return inherited.AsDataDescriptor()->IsWritable();
    }
  }
}

bool JSObject::DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th,
                                 Error* e) {
  // section 8.12.9 [[DefineOwnProperty]]
  Slot slot;
  if (GetOwnPropertySlot(ctx, name, &slot)) {
    // found
    const PropertyDescriptor current = slot.desc();
    assert(!current.IsEmpty());
    bool returned = false;
    if (IsDefineOwnPropertyAccepted(current, desc, th, &returned, e)) {
      if (slot.IsCachable()) {
        GetSlot(slot.offset()) = PropertyDescriptor::Merge(desc, current);
      } else {
        // add property transition
        // searching already created maps and if this is available, move to this
        std::size_t offset;
        map_ = map_->AddPropertyTransition(ctx, name, &offset);
        slots_.resize(map_->GetSlotsSize(), JSUndefined);
        // set newly created property
        GetSlot(offset) = PropertyDescriptor::Merge(desc, current);
      }
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
      // add property transition
      // searching already created maps and if this is available, move to this
      std::size_t offset;
      map_ = map_->AddPropertyTransition(ctx, name, &offset);
      slots_.resize(map_->GetSlotsSize(), JSUndefined);
      // set newly created property
      GetSlot(offset) = PropertyDescriptor::SetDefault(desc);
      return true;
    }
  }
}

void JSObject::Put(Context* ctx,
                   Symbol name,
                   const JSVal& val, bool th, Error* e) {
  if (!CanPut(ctx, name)) {
    if (th) {
      e->Report(Error::Type, "put failed");
    }
    return;
  }
  const PropertyDescriptor own_desc = GetOwnProperty(ctx, name);
  if (!own_desc.IsEmpty() && own_desc.IsDataDescriptor()) {
    DefineOwnProperty(ctx,
                      name,
                      DataDescriptor(
                          val,
                          PropertyDescriptor::UNDEF_ENUMERABLE |
                          PropertyDescriptor::UNDEF_CONFIGURABLE |
                          PropertyDescriptor::UNDEF_WRITABLE), th, e);
    return;
  }
  const PropertyDescriptor desc = GetProperty(ctx, name);
  if (!desc.IsEmpty() && desc.IsAccessorDescriptor()) {
    const AccessorDescriptor* const accs = desc.AsAccessorDescriptor();
    assert(accs->set());
    ScopedArguments args(ctx, 1, IV_LV5_ERROR_VOID(e));
    args[0] = val;
    accs->set()->AsCallable()->Call(&args, this, e);
  } else {
    DefineOwnProperty(ctx, name,
                      DataDescriptor(val,
                                     PropertyDescriptor::WRITABLE |
                                     PropertyDescriptor::ENUMERABLE |
                                     PropertyDescriptor::CONFIGURABLE),
                      th, e);
  }
}

bool JSObject::HasProperty(Context* ctx, Symbol name) const {
  return !GetProperty(ctx, name).IsEmpty();
}

bool JSObject::Delete(Context* ctx, Symbol name, bool th, Error* e) {
  const std::size_t offset = map_->Get(ctx, name);
  if (offset == core::kNotFound) {
    return true;  // not found
  }
  if (!GetSlot(offset).IsConfigurable()) {
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
  GetSlot(offset) = JSUndefined;
  return true;
}

void JSObject::GetPropertyNames(Context* ctx,
                                std::vector<Symbol>* vec,
                                EnumerationMode mode) const {
  GetOwnPropertyNames(ctx, vec, mode);
  const JSObject* obj = prototype_;
  while (obj) {
    obj->GetOwnPropertyNames(ctx, vec, mode);
    obj = obj->prototype();
  }
}

void JSObject::GetOwnPropertyNames(Context* ctx,
                                   std::vector<Symbol>* vec,
                                   EnumerationMode mode) const {
  map_->GetOwnPropertyNames(this, ctx, vec, mode);
}

JSVal JSObject::GetBySlotOffset(Context* ctx, std::size_t n, Error* e) {
  return GetFromDescriptor(ctx, GetSlot(n), e);
}

JSVal JSObject::GetFromDescriptor(Context* ctx,
                                  const PropertyDescriptor& desc, Error* e) {
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

void JSObject::PutToSlotOffset(Context* ctx,
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
      GetSlot(offset) = PropertyDescriptor::Merge(desc, current);
    }
  } else {
    const AccessorDescriptor* const accs = current.AsAccessorDescriptor();
    assert(accs->set());
    ScopedArguments args(ctx, 1, IV_LV5_ERROR_VOID(e));
    args[0] = val;
    accs->set()->AsCallable()->Call(&args, this, e);
  }
}

bool JSObject::GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
  const std::size_t offset = map()->Get(ctx, name);
  if (offset != core::kNotFound) {
    slot->set_descriptor(GetSlot(offset), this, offset);
    return true;
  }
  return false;
}

JSObject* JSObject::New(Context* ctx) {
  JSObject* const obj = NewPlain(ctx);
  obj->set_cls(JSObject::GetClass());
  obj->set_prototype(context::GetClassSlot(ctx, Class::Object).prototype);
  return obj;
}

JSObject* JSObject::New(Context* ctx, Map* map) {
  JSObject* const obj = NewPlain(ctx, map);
  obj->set_cls(JSObject::GetClass());
  obj->set_prototype(context::GetClassSlot(ctx, Class::Object).prototype);
  return obj;
}

Map* JSObject::FlattenMap() const {
  // make map transitable
  map_->Flatten();
  return map_;
}

JSObject* JSObject::NewPlain(Context* ctx) {
  return new JSObject(context::GetEmptyObjectMap(ctx));
}

JSObject* JSObject::NewPlain(Context* ctx, Map* map) {
  return new JSObject(map);
}

} }  // namespace iv::lv5
