#include <cassert>
#include <algorithm>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/property.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/context.h>
#include <iv/lv5/class.h>
#include <iv/lv5/jsbooleanobject.h>
#include <iv/lv5/jsnumberobject.h>
#include <iv/lv5/map.h>
#include <iv/lv5/slot.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/radio/radio.h>
namespace iv {
namespace lv5 {

JSObject::JSObject(Map* map)
  : cls_(NULL),
    map_(map),
    prototype_(NULL),
    slots_(map->GetSlotsSize()),
    flags_() {
  set_extensible(true);
}

JSObject::JSObject(Map* map,
                   JSObject* proto,
                   Class* cls,
                   bool extensible)
  : cls_(cls),
    map_(map),
    prototype_(proto),
    slots_(map->GetSlotsSize()),
    flags_() {
  set_extensible(extensible);
}

#define TRY(context, sym, arg, e)\
  do {\
    const JSVal method = Get(context, sym, IV_LV5_ERROR(e));\
    if (method.IsCallable()) {\
      const JSVal val =\
        static_cast<JSFunction*>(\
            method.object())->Call(&arg, this, IV_LV5_ERROR(e));\
      if (val.IsPrimitive() || val.IsNullOrUndefined()) {\
        return val;\
      }\
    }\
  } while (0)
JSVal JSObject::DefaultValue(Context* ctx, Hint::Object hint, Error* e) {
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

JSVal JSObject::Get(Context* ctx, Symbol name, Error* e) {
  Slot slot;
  return GetSlot(ctx, name, &slot, e);
}

JSVal JSObject::GetSlot(Context* ctx, Symbol name, Slot* slot, Error* e) {
  if (GetPropertySlot(ctx, name, slot)) {
    return slot->Get(ctx, this, e);
  }
  return JSUndefined;
}

// not recursion
PropertyDescriptor JSObject::GetProperty(Context* ctx, Symbol name) const {
  Slot slot;
  if (GetPropertySlot(ctx, name, &slot)) {
    return slot.ToDescriptor();
  }
  return JSEmpty;
}

bool JSObject::GetPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
  const JSObject* obj = this;
  do {
    if (obj->GetOwnPropertySlot(ctx, name, slot)) {
      assert(!slot->IsNotFound());
      return true;
    }
    obj = obj->prototype();
  } while (obj);
  return false;
}

PropertyDescriptor JSObject::GetOwnProperty(Context* ctx, Symbol name) const {
  Slot slot;
  if (GetOwnPropertySlot(ctx, name, &slot)) {
    assert(!slot.IsNotFound());
    return slot.ToDescriptor();
  }
  return JSEmpty;
}

bool JSObject::CanPut(Context* ctx, Symbol name) const {
  {
    Slot slot;
    if (GetOwnPropertySlot(ctx, name, &slot)) {
      if (slot.attributes().IsAccessor()) {
        return slot.accessor()->setter();
      } else {
        assert(slot.attributes().IsData());
        return slot.attributes().IsWritable();
      }
    }
  }
  if (!prototype()) {
    return IsExtensible();
  }
  {
    Slot inherited;
    if (prototype()->GetPropertySlot(ctx, name, &inherited)) {
      if (inherited.attributes().IsAccessor()) {
        return inherited.accessor()->setter();
      } else {
        assert(inherited.attributes().IsData());
        return inherited.attributes().IsWritable();
      }
    } else {
      return IsExtensible();
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
    bool returned = false;
    if (slot.IsDefineOwnPropertyAccepted(desc, th, &returned, e)) {
      if (slot.HasOffset()) {
        const Attributes::Safe old(slot.attributes());
        slot.Merge(ctx, desc);
        if (old != slot.attributes()) {
          set_map(map()->ChangeAttributesTransition(ctx, name, slot.attributes()));
        }
        Direct(slot.offset()) = slot.value();
      } else {
        // add property transition
        // searching already created maps and if this is available, move to this
        uint32_t offset;
        slot.Merge(ctx, desc);
        set_map(map()->AddPropertyTransition(ctx, name, slot.attributes(), &offset));
        slots_.resize(map()->GetSlotsSize(), JSEmpty);
        // set newly created property
        Direct(offset) = slot.value();
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
      // set newly created property
      // searching already created maps and if this is available, move to this
      uint32_t offset;
      const StoredSlot stored(ctx, desc);
      set_map(map()->AddPropertyTransition(ctx, name, stored.attributes(), &offset));
      slots_.resize(map()->GetSlotsSize(), JSEmpty);
      Direct(offset) = stored.value();
      return true;
    }
  }
}

void JSObject::Put(Context* ctx, Symbol name, JSVal val, bool th, Error* e) {
  if (!CanPut(ctx, name)) {
    if (th) {
      e->Report(Error::Type, "put failed");
    }
    return;
  }

  Slot slot;
  if (GetPropertySlot(ctx, name, &slot)) {
    // own property and attributes is data
    if (slot.base() == this && slot.attributes().IsData()) {
      DefineOwnProperty(
          ctx,
          name,
          DataDescriptor(val,
                         ATTR::UNDEF_ENUMERABLE |
                         ATTR::UNDEF_CONFIGURABLE |
                         ATTR::UNDEF_WRITABLE),
          th, e);
      return;
    }

    // accessor is found
    if (slot.attributes().IsAccessor()) {
      const Accessor* ac = slot.accessor();
      assert(ac->setter());
      ScopedArguments args(ctx, 1, IV_LV5_ERROR_VOID(e));
      args[0] = val;
      static_cast<JSFunction*>(ac->setter())->Call(&args, this, e);
      return;
    }
  }
  // not found or found but data property.
  // create or modify data property
  DefineOwnProperty(
      ctx, name,
      DataDescriptor(val, ATTR::W | ATTR::E | ATTR::C), th, e);
}

bool JSObject::HasProperty(Context* ctx, Symbol name) const {
  Slot slot;
  return GetPropertySlot(ctx, name, &slot);
}

// Delete direct doesn't lookup by GetOwnPropertySlot.
// Simple, lookup from map and delete it
bool JSObject::DeleteDirect(Context* ctx, Symbol name, bool th, Error* e) {
  const Map::Entry entry = map()->Get(ctx, name);
  if (entry.IsNotFound()) {
    return true;  // not found
  }
  if (!entry.attributes.IsConfigurable()) {
    if (th) {
      e->Report(Error::Type, "delete failed");
    }
    return false;
  }

  // delete property transition
  // if previous map is avaiable shape, move to this.
  // and if that is not avaiable, create new map and move to it.
  // newly created slots size is always smaller than before
  set_map(map()->DeletePropertyTransition(ctx, name));
  Direct(entry.offset) = JSEmpty;
  return true;
}


bool JSObject::Delete(Context* ctx, Symbol name, bool th, Error* e) {
  Slot slot;
  if (!GetOwnPropertySlot(ctx, name, &slot)) {
    return true;
  }

  if (!slot.attributes().IsConfigurable()) {
    if (th) {
      e->Report(Error::Type, "delete failed");
    }
    return false;
  }

  // If target is JSNormalArguments,
  // Descriptor maybe configurable but not cacheable.
  uint32_t offset;
  if (slot.HasOffset()) {
    offset = slot.offset();
  } else {
    const Map::Entry entry = map()->Get(ctx, name);
    if (entry.IsNotFound()) {
      return true;
    }
    offset = entry.offset;
  }

  // delete property transition
  // if previous map is avaiable shape, move to this.
  // and if that is not avaiable, create new map and move to it.
  // newly created slots size is always smaller than before
  set_map(map()->DeletePropertyTransition(ctx, name));
  Direct(offset) = JSEmpty;
  return true;
}

void JSObject::GetPropertyNames(Context* ctx,
                                PropertyNamesCollector* collector,
                                EnumerationMode mode) const {
  GetOwnPropertyNames(ctx, collector, mode);
  const JSObject* obj = prototype();
  while (obj) {
    obj->GetOwnPropertyNames(ctx, collector->LevelUp(), mode);
    obj = obj->prototype();
  }
}

void JSObject::GetOwnPropertyNames(Context* ctx,
                                   PropertyNamesCollector* collector,
                                   EnumerationMode mode) const {
  map()->GetOwnPropertyNames(collector, mode);
}

bool JSObject::GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
  const Map::Entry entry = map()->Get(ctx, name);
  if (!entry.IsNotFound()) {
    slot->set(Direct(entry.offset), entry.attributes, this, entry.offset);
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

void JSObject::MarkChildren(radio::Core* core) {
  core->MarkCell(prototype_);
  core->MarkCell(map_);
  std::for_each(slots_.begin(), slots_.end(), radio::Core::Marker(core));
}

} }  // namespace iv::lv5
