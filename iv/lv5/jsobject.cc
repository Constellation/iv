#include <cassert>
#include <algorithm>
#include <iv/lv5/jsobject.h>
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
    slots_(map->GetSlotsSize()),
    elements_(),
    flags_(kFlagExtensible) {
}

JSObject::JSObject(const JSObject& obj)
  : cls_(obj.cls_),
    map_(obj.map_),
    slots_(obj.slots_),
    elements_(obj.elements_),
    flags_(obj.flags_) {
}

JSObject::JSObject(Map* map, Class* cls)
  : cls_(cls),
    map_(map),
    slots_(map->GetSlotsSize()),
    elements_(),
    flags_(kFlagExtensible) {
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

bool JSObject::HasOwnProperty(Context* ctx, Symbol name) const {
  Slot slot;
  return GetOwnPropertySlot(ctx, name, &slot);
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

bool JSObject::CanPut(Context* ctx, Symbol name, Slot* slot) const {
  if (GetPropertySlot(ctx, name, slot)) {
    if (slot->attributes().IsAccessor()) {
      return slot->accessor()->setter();
    } else {
      assert(slot->attributes().IsData());
      return slot->attributes().IsWritable();
    }
  }
  return IsExtensible();
}

bool JSObject::DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th,
                                 Error* e) {
  Slot slot;
  return DefineOwnPropertySlot(ctx, name, desc, &slot, th, e);
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
  if (desc.IsAccessor()) {
    return false;
  }
  if (desc.IsData()) {
    const DataDescriptor* const data = desc.AsDataDescriptor();
    return data->IsWritable() || data->IsWritableAbsent();
  }
  return true;
}

// section 8.12.9 [[DefineOwnProperty]]
bool JSObject::DefineOwnPropertySlot(Context* ctx,
                                     Symbol name,
                                     const PropertyDescriptor& desc,
                                     Slot* slot, bool th, Error* e) {
  if (symbol::IsArrayIndexSymbol(name)) {
    return DefineOwnIndexedPropertyInternal(ctx, symbol::GetIndexFromSymbol(name), desc, th, e);
  }

  if (!slot->IsUsed()) {
    GetOwnPropertySlot(ctx, name, slot);
  }

  if (!slot->IsNotFound() && slot->base() == this) {
    // found
    bool returned = false;
    if (slot->IsDefineOwnPropertyAccepted(desc, th, &returned, e)) {
      if (slot->HasOffset()) {
        const Attributes::Safe old(slot->attributes());
        slot->Merge(ctx, desc);
        if (old != slot->attributes()) {
          set_map(map()->ChangeAttributesTransition(ctx, name, slot->attributes()));
        }
        Direct(slot->offset()) = slot->value();
        slot->MarkPutResult(Slot::PUT_REPLACE, slot->offset());
      } else {
        // add property transition
        // searching already created maps and if this is available, move to this
        uint32_t offset;
        slot->Merge(ctx, desc);
        set_map(map()->AddPropertyTransition(ctx, name, slot->attributes(), &offset));
        slots_.resize(map()->GetSlotsSize(), JSEmpty);
        // set newly created property
        Direct(offset) = slot->value();
        slot->MarkPutResult(Slot::PUT_NEW, offset);
      }
    }
    return returned;
  }

  // not found
  if (!IsExtensible()) {
    if (th) {
      e->Report(Error::Type, "object not extensible");\
    }
    return false;
  }

  // add property transition
  // set newly created property
  // searching already created maps and if this is available, move to this
  uint32_t offset;
  const StoredSlot stored(ctx, desc);
  set_map(map()->AddPropertyTransition(ctx, name, stored.attributes(), &offset));
  slots_.resize(map()->GetSlotsSize(), JSEmpty);
  Direct(offset) = stored.value();
  slot->MarkPutResult(Slot::PUT_NEW, offset);
  return true;
}

void JSObject::Put(Context* ctx, Symbol name, JSVal val, bool th, Error* e) {
  Slot slot;
  PutSlot(ctx, name, val, &slot, th, e);
}

void JSObject::PutSlot(Context* ctx, Symbol name, JSVal val, Slot* slot, bool th, Error* e) {
  if (!CanPut(ctx, name, slot)) {
    if (th) {
      e->Report(Error::Type, "put failed");
    }
    return;
  }

  if (!slot->IsNotFound()) {
    // own property and attributes is data
    if (slot->base() == this && slot->attributes().IsData()) {
      DefineOwnPropertySlot(
          ctx,
          name,
          DataDescriptor(val,
                         ATTR::UNDEF_ENUMERABLE |
                         ATTR::UNDEF_CONFIGURABLE |
                         ATTR::UNDEF_WRITABLE),
          slot,
          th, e);
      return;
    }

    // accessor is found
    if (slot->attributes().IsAccessor()) {
      const Accessor* ac = slot->accessor();
      assert(ac->setter());
      ScopedArguments args(ctx, 1, IV_LV5_ERROR_VOID(e));
      args[0] = val;
      static_cast<JSFunction*>(ac->setter())->Call(&args, this, e);
      return;
    }
  }
  // not found or found but data property.
  // create or modify data property
  DefineOwnPropertySlot(
      ctx, name,
      DataDescriptor(val, ATTR::W | ATTR::E | ATTR::C), slot, th, e);
}

bool JSObject::HasProperty(Context* ctx, Symbol name) const {
  Slot slot;
  return GetPropertySlot(ctx, name, &slot);
}

bool JSObject::Delete(Context* ctx, Symbol name, bool th, Error* e) {
  if (symbol::IsArrayIndexSymbol(name)) {
    return DeleteIndexedInternal(ctx, symbol::GetIndexFromSymbol(name), th, e);
  }

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
  uint32_t index = 0;
  for (DenseArrayVector::const_iterator it = elements_.vector.begin(),
       last = elements_.vector.end(); it != last; ++it, ++index) {
    if (!it->IsEmpty()) {
      collector->Add(index);
    }
  }
  if (elements_.map) {
    for (SparseArrayMap::const_iterator it = elements_.map->begin(),
         last = elements_.map->end(); it != last; ++it) {
      collector->Add(it->first);
    }
  }
  map()->GetOwnPropertyNames(collector, mode);
}

bool JSObject::GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
  if (symbol::IsArrayIndexSymbol(name)) {
    return GetOwnIndexedPropertySlotInternal(ctx, symbol::GetIndexFromSymbol(name), slot);
  }
  const Map::Entry entry = map()->Get(ctx, name);
  if (!entry.IsNotFound()) {
    slot->set(Direct(entry.offset), entry.attributes, this, entry.offset);
    return true;
  }
  return false;
}

void JSObject::ChangePrototype(Context* ctx, JSObject* proto) {
  set_map(map()->ChangePrototypeTransition(ctx, proto));
}

bool JSObject::DeleteIndexedInternal(Context* ctx, uint32_t index, bool th, Error* e) {
  if (elements_.length() <= index) {
    return true;
  }

  if (index < elements_.vector.size()) {
    elements_.vector[index] = JSEmpty;
    return true;
  }

  if (index < IndexedElements::kMaxVectorSize) {
    return true;
  }

  if (!elements_.map) {
    return true;
  }

  SparseArrayMap* sparse = elements_.map;
  SparseArrayMap::iterator it = sparse->find(index);
  if (it != sparse->end()) {
    return true;
  }

  if (!it->second.attributes().IsConfigurable()) {
    if (th) {
      e->Report(Error::Type, "delete failed");
    }
    return false;
  }

  sparse->erase(it);
  if (sparse->empty()) {
    elements_.MakeDense();
  }
  return true;
}

bool JSObject::GetOwnIndexedPropertySlotInternal(Context* ctx, uint32_t index, Slot* slot) const {
  if (index < elements_.vector.size()) {
    const JSVal value = elements_.vector[index];
    if (value.IsEmpty()) {
      return false;
    }
    slot->set(value, ATTR::Object::Data(), this);
    return true;
  }
  if (elements_.map && index < elements_.length()) {
    const SparseArrayMap::const_iterator it = elements_.map->find(index);
    if (it != elements_.map->end()) {
      slot->set(it->second, this);
      return true;
    }
  }
  return false;
}

bool JSObject::DefineOwnIndexedPropertyInternal(Context* ctx, uint32_t index,
                                                const PropertyDescriptor& desc, bool th, Error* e) {
  if (index >= elements_.length() && !elements_.writable()) {
    if (th) {
      e->Report(Error::Type,
                "adding an element to the array "
                "which length is not writable is rejected");
    }
    return false;
  }

  if (elements_.dense()) {
    assert(IsExtensible());
    const bool absent = IsAbsentDescriptor(desc);
    if (desc.IsDefault() || absent) {
      // fast path
      JSVal value = JSUndefined;
      if (!desc.AsDataDescriptor()->IsValueAbsent()) {
        value = desc.AsDataDescriptor()->value();
      }

      if (index < elements_.vector.size()) {
        elements_.vector[index] = value;
        return true;
      }

      if (absent) {
        // purge vector
        elements_.MakeSparse();
      } else {
        if (index < IndexedElements::kMaxVectorSize) {
          elements_.vector.resize(index + 1, JSEmpty);
          elements_.vector[index] = value;
          if (index >= elements_.length()) {
            elements_.set_length(index + 1);
          }
          return true;
        }
      }
    } else {
      if (index < IndexedElements::kMaxVectorSize) {
        // purge vector
        elements_.MakeSparse();
      }
    }
  }

  SparseArrayMap* sparse = elements_.EnsureMap();
  SparseArrayMap::iterator it = sparse->find(index);
  if (it != sparse->end()) {
    StoredSlot merge(it->second);
    bool returned = false;
    if (merge.IsDefineOwnPropertyAccepted(desc, th, &returned, e)) {
      merge.Merge(ctx, desc);
      (*sparse)[index] = merge;
    }
    return returned;
  }

  // new element
  if (!IsExtensible()) {
    if (th) {
      e->Report(Error::Type, "object not extensible");\
    }
    return false;
  }
  if (index >= elements_.length()) {
    elements_.set_length(index + 1);
  }
  sparse->insert(std::make_pair(index, StoredSlot(ctx, desc)));
  return true;
}

JSObject* JSObject::New(Context* ctx) {
  JSObject* const obj = NewPlain(ctx, ctx->global_data()->empty_object_map());
  obj->set_cls(JSObject::GetClass());
  return obj;
}

JSObject* JSObject::New(Context* ctx, Map* map) {
  JSObject* const obj = NewPlain(ctx, map);
  obj->set_cls(JSObject::GetClass());
  return obj;
}

Map* JSObject::FlattenMap() const {
  // make map transitable
  map_->Flatten();
  return map_;
}

JSObject* JSObject::NewPlain(Context* ctx, Map* map) {
  return new JSObject(map);
}

void JSObject::MapTransitionWithReallocation(
    JSObject* base, JSVal src, Map* transit, uint32_t offset) {
  base->set_map(transit);
  base->slots_.resize(transit->GetSlotsSize(), JSEmpty);
  base->Direct(offset) = src;
}

void JSObject::MarkChildren(radio::Core* core) {
  core->MarkCell(map_);
  std::for_each(slots_.begin(), slots_.end(), radio::Core::Marker(core));
  std::for_each(elements_.vector.begin(), elements_.vector.end(), radio::Core::Marker(core));
  if (elements_.map) {
    for (SparseArrayMap::const_iterator it = elements_.map->begin(),
         last = elements_.map->end(); it != last; ++it) {
      core->MarkValue(it->second.value());
    }
  }
}

void JSObject::ChangeExtensible(Context* ctx, bool val) {
  if (val) {
    flags_ |= kFlagExtensible;
  } else {
    flags_ &= ~kFlagExtensible;
  }
  set_map(map()->ChangeExtensibleTransition(ctx));
  elements_.MakeSparse();
}


JSObject* JSObject::prototype() const { return map()->prototype(); }

} }  // namespace iv::lv5
