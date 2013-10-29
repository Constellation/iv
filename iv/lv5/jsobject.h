#ifndef IV_LV5_JSOBJECT_H_
#define IV_LV5_JSOBJECT_H_
#include <cassert>
#include <algorithm>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jsobject_interface.h>
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

inline bool IsAbsentDescriptor(const PropertyDescriptor& desc) {
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

inline JSObject::JSObject(Map* map)
  : JSCell(radio::OBJECT, map, NULL),
    slots_(map->GetSlotsSize()),
    elements_(),
    flags_(kFlagExtensible) {
}

inline JSObject::JSObject(const JSObject& obj)
  : JSCell(obj),
    slots_(obj.slots_),
    elements_(obj.elements_),
    flags_(obj.flags_) {
}

inline JSObject::JSObject(Map* map, const Class* cls)
  : JSCell(radio::OBJECT, map, cls),
    slots_(map->GetSlotsSize()),
    elements_(),
    flags_(kFlagExtensible) {
}

inline JSVal JSObject::Get(Context* ctx, Symbol name, Error* e) {
  Slot slot;
  return GetSlot(ctx, name, &slot, e);
}

inline PropertyDescriptor JSObject::GetOwnProperty(Context* ctx, Symbol name) const {
  Slot slot;
  if (GetOwnPropertySlot(ctx, name, &slot)) {
    assert(!slot.IsNotFound());
    return slot.ToDescriptor();
  }
  return JSEmpty;
}

inline PropertyDescriptor JSObject::GetProperty(Context* ctx, Symbol name) const {
  Slot slot;
  if (GetPropertySlot(ctx, name, &slot)) {
    return slot.ToDescriptor();
  }
  return JSEmpty;
}

inline bool JSObject::HasOwnProperty(Context* ctx, Symbol name) const {
  Slot slot;
  return GetOwnPropertySlot(ctx, name, &slot);
}

inline void JSObject::Put(Context* ctx, Symbol name, JSVal val, bool throwable, Error* e) {
  Slot slot;
  PutSlot(ctx, name, val, &slot, throwable, e);
}

inline bool JSObject::DefineOwnProperty(Context* ctx,
                                        Symbol name,
                                        const PropertyDescriptor& desc,
                                        bool throwable,
                                        Error* e) {
  Slot slot;
  return DefineOwnPropertySlot(ctx, name, desc, &slot, throwable, e);
}

inline bool JSObject::HasProperty(Context* ctx, Symbol name) const {
  Slot slot;
  return GetPropertySlot(ctx, name, &slot);
}

inline bool JSObject::CanPutNonIndexed(Context* ctx, Symbol name, Slot* slot) const {
  if (GetNonIndexedPropertySlot(ctx, name, slot)) {
    if (slot->attributes().IsAccessor()) {
      return slot->accessor()->setter();
    } else {
      assert(slot->attributes().IsData());
      return slot->attributes().IsWritable();
    }
  }
  return IsExtensible();
}

inline bool JSObject::CanPutIndexed(Context* ctx, uint32_t index, Slot* slot) const {
  if (GetIndexedPropertySlot(ctx, index, slot)) {
    if (slot->attributes().IsAccessor()) {
      return slot->accessor()->setter();
    } else {
      assert(slot->attributes().IsData());
      return slot->attributes().IsWritable();
    }
  }
  return IsExtensible();
}

// real code

inline JSVal JSObject::GetNonIndexedSlotMethod(JSObject* obj, Context* ctx, Symbol name, Slot* slot, Error* e) {
  if (obj->GetNonIndexedPropertySlot(ctx, name, slot)) {
    return slot->Get(ctx, obj, e);
  }
  return JSUndefined;
}

inline JSVal JSObject::GetIndexedSlotMethod(JSObject* obj, Context* ctx, uint32_t name, Slot* slot, Error* e) {
  if (obj->GetIndexedPropertySlot(ctx, name, slot)) {
    return slot->Get(ctx, obj, e);
  }
  return JSUndefined;
}

inline bool JSObject::GetNonIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, Symbol name, Slot* slot) {
  do {
    if (obj->GetOwnNonIndexedPropertySlot(ctx, name, slot)) {
      assert(!slot->IsNotFound());
      return true;
    }
    obj = obj->prototype();
  } while (obj);
  slot->MakeUsed();
  return false;
}

inline bool JSObject::GetIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, uint32_t index, Slot* slot) {
  do {
    if (obj->GetOwnIndexedPropertySlot(ctx, index, slot)) {
      assert(!slot->IsNotFound());
      return true;
    }
    obj = obj->prototype();
  } while (obj);
  slot->MakeUsed();
  return false;
}

inline bool JSObject::GetOwnNonIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, Symbol name, Slot* slot) {
  const Map::Entry entry = obj->map()->Get(ctx, name);
  if (!entry.IsNotFound()) {
    slot->set(obj->Direct(entry.offset), entry.attributes, obj, entry.offset);
    return true;
  }
  return false;
}

inline bool JSObject::GetOwnIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, uint32_t index, Slot* slot) {
  if (obj->elements_.dense() && index < obj->elements_.vector.size()) {
    const JSVal value = obj->elements_.vector[index];
    if (value.IsEmpty()) {
      return false;
    }
    slot->set(value, ATTR::Object::Data(), obj);
    return true;
  }
  if (obj->elements_.map && index < obj->elements_.length()) {
    const SparseArrayMap::const_iterator it = obj->elements_.map->find(index);
    if (it != obj->elements_.map->end()) {
      slot->set(it->second, obj);
      return true;
    }
  }
  return false;
}

// section 8.12.9 [[DefineOwnProperty]]
inline bool JSObject::DefineOwnNonIndexedPropertySlotMethod(JSObject* obj,
                                                            Context* ctx,
                                                            Symbol name,
                                                            const PropertyDescriptor& desc,
                                                            Slot* slot, bool throwable, Error* e) {
  if (!slot->IsUsed()) {
    obj->GetOwnPropertySlot(ctx, name, slot);
  }

  if (!slot->IsNotFound() && slot->base() == obj) {
    // found
    bool returned = false;
    if (slot->IsDefineOwnPropertyAccepted(desc, throwable, &returned, e)) {
      if (slot->HasOffset()) {
        const Attributes::Safe old(slot->attributes());
        slot->Merge(ctx, desc);
        if (old != slot->attributes()) {
          obj->set_map(obj->map()->ChangeAttributesTransition(ctx, name, slot->attributes()));
        }
        obj->Direct(slot->offset()) = slot->value();
        slot->MarkPutResult(Slot::PUT_REPLACE, slot->offset());
      } else {
        // add property transition
        // searching already created maps and if this is available, move to this
        uint32_t offset;
        slot->Merge(ctx, desc);
        obj->set_map(obj->map()->AddPropertyTransition(ctx, name, slot->attributes(), &offset));
        obj->slots_.resize(obj->map()->GetSlotsSize(), JSEmpty);
        // set newly created property
        obj->Direct(offset) = slot->value();
        slot->MarkPutResult(Slot::PUT_NEW, offset);
      }
    }
    return returned;
  }

  // not found
  if (!obj->IsExtensible()) {
    if (throwable) {
      e->Report(Error::Type, "object not extensible");\
    }
    return false;
  }

  // add property transition
  // set newly created property
  // searching already created maps and if this is available, move to this
  uint32_t offset;
  const StoredSlot stored(ctx, desc);
  obj->set_map(obj->map()->AddPropertyTransition(ctx, name, stored.attributes(), &offset));
  obj->slots_.resize(obj->map()->GetSlotsSize(), JSEmpty);
  obj->Direct(offset) = stored.value();
  slot->MarkPutResult(Slot::PUT_NEW, offset);
  return true;
}

inline bool JSObject::DefineOwnIndexedPropertySlotMethod(JSObject* obj,
                                                         Context* ctx,
                                                         uint32_t index,
                                                         const PropertyDescriptor& desc,
                                                         Slot* slot, bool throwable, Error* e) {
  if (obj->method()->GetOwnIndexedPropertySlot != JSObject::GetOwnIndexedPropertySlotMethod) {
    // We should reject following case
    //   var str = new String('str');
    //   Object.defineProperty(str, '0', { value: 0 });
    if (!slot->IsUsed()) {
      obj->GetOwnIndexedPropertySlot(ctx, index, slot);
    }

    bool returned = false;
    if (!slot->IsNotFound() && slot->base() == obj) {
      if (!slot->IsDefineOwnPropertyAccepted(desc, throwable, &returned, e)) {
        return returned;
      }
      // through
    }
  }
  return obj->DefineOwnIndexedPropertyInternal(ctx, index, desc, throwable, e);
}

inline void JSObject::PutNonIndexedSlotMethod(JSObject* obj, Context* ctx, Symbol name, JSVal val, Slot* slot, bool throwable, Error* e) {
  if (!obj->CanPut(ctx, name, slot)) {
    if (throwable) {
      e->Report(Error::Type, "put failed");
    }
    return;
  }

  if (!slot->IsNotFound()) {
    // own property and attributes is data
    if (slot->base() == obj && slot->attributes().IsData()) {
      obj->DefineOwnNonIndexedPropertySlot(
          ctx,
          name,
          DataDescriptor(val,
                         ATTR::UNDEF_ENUMERABLE |
                         ATTR::UNDEF_CONFIGURABLE |
                         ATTR::UNDEF_WRITABLE),
          slot,
          throwable, e);
      return;
    }

    // accessor is found
    if (slot->attributes().IsAccessor()) {
      const Accessor* ac = slot->accessor();
      assert(ac->setter());
      ScopedArguments args(ctx, 1, IV_LV5_ERROR_VOID(e));
      args[0] = val;
      static_cast<JSFunction*>(ac->setter())->Call(&args, obj, e);
      return;
    }
  }
  // not found or found but data property.
  // create or modify data property
  obj->DefineOwnNonIndexedPropertySlot(
      ctx, name,
      DataDescriptor(val, ATTR::W | ATTR::E | ATTR::C), slot, throwable, e);
}

inline void JSObject::PutIndexedSlotMethod(JSObject* obj, Context* ctx, uint32_t index, JSVal val, Slot* slot, bool throwable, Error* e) {
  if (index < IndexedElements::kMaxVectorSize &&
      obj->elements_.dense() &&
      obj->method()->GetOwnIndexedPropertySlot == JSObject::GetOwnIndexedPropertySlotMethod &&
      (!obj->prototype() || !obj->prototype()->HasIndexedProperty())) {
    // array fast path
    slot->MarkPutResult(Slot::PUT_INDEXED_OPTIMIZED, index);
    obj->DefineOwnIndexedValueDenseInternal(ctx, index, val, false);
    return;
  }

  if (!obj->CanPutIndexed(ctx, index, slot)) {
    if (throwable) {
      e->Report(Error::Type, "put failed");
    }
    return;
  }

  if (!slot->IsNotFound()) {
    // own property and attributes is data
    if (slot->base() == obj && slot->attributes().IsData()) {
      obj->DefineOwnIndexedPropertySlot(
          ctx,
          index,
          DataDescriptor(val,
                         ATTR::UNDEF_ENUMERABLE |
                         ATTR::UNDEF_CONFIGURABLE |
                         ATTR::UNDEF_WRITABLE),
          slot,
          throwable, e);
      return;
    }

    // accessor is found
    if (slot->attributes().IsAccessor()) {
      const Accessor* ac = slot->accessor();
      assert(ac->setter());
      ScopedArguments args(ctx, 1, IV_LV5_ERROR_VOID(e));
      args[0] = val;
      static_cast<JSFunction*>(ac->setter())->Call(&args, obj, e);
      return;
    }
  }
  // not found or found but data property.
  // create or modify data property
  obj->DefineOwnIndexedPropertySlot(
      ctx, index,
      DataDescriptor(val, ATTR::W | ATTR::E | ATTR::C), slot, throwable, e);
}

inline bool JSObject::DeleteNonIndexedMethod(JSObject* obj, Context* ctx, Symbol name, bool throwable, Error* e) {
  Slot slot;
  if (!obj->GetOwnPropertySlot(ctx, name, &slot)) {
    return true;
  }

  if (!slot.attributes().IsConfigurable()) {
    if (throwable) {
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
    const Map::Entry entry = obj->map()->Get(ctx, name);
    if (entry.IsNotFound()) {
      return true;
    }
    offset = entry.offset;
  }

  // delete property transition
  // if previous map is avaiable shape, move to this.
  // and if that is not avaiable, create new map and move to it.
  // newly created slots size is always smaller than before
  obj->set_map(obj->map()->DeletePropertyTransition(ctx, name));
  obj->Direct(offset) = JSEmpty;
  return true;
}

inline bool JSObject::DeleteIndexedMethod(JSObject* obj, Context* ctx, uint32_t index, bool throwable, Error* e) {
  // fast path
  if (obj->method()->GetOwnIndexedPropertySlot == JSObject::GetOwnIndexedPropertySlotMethod) {
    return obj->DeleteIndexedInternal(ctx, index, throwable, e);
  }

  // We should raise error to following case
  //   var str = new String('test');
  //   (function () {
  //     'use strict';
  //     delete str[0];
  //   }());
  Slot slot;
  if (!obj->method()->GetOwnIndexedPropertySlot(obj, ctx, index, &slot)) {
    return true;
  }

  if (!slot.attributes().IsConfigurable()) {
    if (throwable) {
      e->Report(Error::Type, "try to delete not configurable property");
    }
    return false;
  }
  return obj->DeleteIndexedInternal(ctx, index, throwable, e);
}

inline void JSObject::GetPropertyNamesMethod(const JSObject* obj,
                                             Context* ctx,
                                             PropertyNamesCollector* collector,
                                             EnumerationMode mode) {
  obj->GetOwnPropertyNames(ctx, collector, mode);
  obj = obj->prototype();
  while (obj) {
    obj->GetOwnPropertyNames(ctx, collector->LevelUp(), mode);
    obj = obj->prototype();
  }
}

inline void JSObject::GetOwnPropertyNamesMethod(const JSObject* obj,
                                                Context* ctx,
                                                PropertyNamesCollector* collector,
                                                EnumerationMode mode) {
  uint32_t index = 0;
  if (obj->elements_.dense()) {
    for (DenseArrayVector::const_iterator it = obj->elements_.vector.begin(),
         last = obj->elements_.vector.end(); it != last; ++it, ++index) {
      if (!it->IsEmpty()) {
        collector->Add(index);
      }
    }
  }
  if (obj->elements_.map) {
    for (SparseArrayMap::const_iterator it = obj->elements_.map->begin(),
         last = obj->elements_.map->end(); it != last; ++it) {
      if (mode == INCLUDE_NOT_ENUMERABLE || it->second.attributes().IsEnumerable()) {
        collector->Add(it->first);
      }
    }
  }
  obj->map()->GetOwnPropertyNames(collector, mode);
}

#define TRY(context, sym, arg, e)\
  do {\
    const JSVal m = obj->Get(context, sym, IV_LV5_ERROR(e));\
    if (m.IsCallable()) {\
      const JSVal val =\
        static_cast<JSFunction*>(m.object())->Call(&arg, obj, IV_LV5_ERROR(e));\
      if (val.IsPrimitive() || val.IsNullOrUndefined()) {\
        return val;\
      }\
    }\
  } while (0)
inline JSVal JSObject::DefaultValueMethod(JSObject* obj, Context* ctx, Hint::Object hint, Error* e) {
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

inline void JSObject::ChangePrototype(Context* ctx, JSObject* proto) {
  set_map(map()->ChangePrototypeTransition(ctx, proto));
}

inline bool JSObject::DeleteIndexedInternal(Context* ctx, uint32_t index, bool throwable, Error* e) {
  if (elements_.length() <= index) {
    return true;
  }

  if (elements_.dense()) {
    if (index < elements_.vector.size()) {
      elements_.vector[index] = JSEmpty;
      return true;
    }

    if (index < IndexedElements::kMaxVectorSize) {
      return true;
    }
  }

  if (!elements_.map) {
    return true;
  }

  SparseArrayMap* sparse = elements_.map;
  SparseArrayMap::iterator it = sparse->find(index);
  if (it == sparse->end()) {
    return true;
  }

  if (!it->second.attributes().IsConfigurable()) {
    if (throwable) {
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

inline void JSObject::DefineOwnIndexedValueDenseInternal(Context* ctx, uint32_t index, JSVal value, bool absent) {
  assert(elements_.dense());
  assert(index < IndexedElements::kMaxVectorSize);
  // fast path
  if (index < elements_.vector.size()) {
    assert(map()->IsIndexed());
    if (!absent) {
      elements_.vector[index] = value;
    } else if (elements_.vector[index].IsEmpty()) {
      elements_.vector[index] = JSUndefined;
    }
  } else {
    if (!map()->IsIndexed()) {
      set_map(map()->ChangeIndexedTransition(ctx));
    }
    elements_.vector.resize(index + 1, JSEmpty);
    if (!absent) {
      elements_.vector[index] = value;
    } else {
      elements_.vector[index] = JSUndefined;
    }
  }
  if (index >= elements_.length()) {
    elements_.set_length(index + 1);
  }
}

inline bool JSObject::DefineOwnIndexedPropertyInternal(Context* ctx, uint32_t index,
                                                       const PropertyDescriptor& desc, bool throwable, Error* e) {
  if (index >= elements_.length() && !elements_.writable()) {
    if (throwable) {
      e->Report(Error::Type,
                "adding an element to the array "
                "which length is not writable is rejected");
    }
    return false;
  }

  if (elements_.dense()) {
    assert(IsExtensible());

    if (desc.IsDefault()) {
      // fast path
      if (index < IndexedElements::kMaxVectorSize) {
        DefineOwnIndexedValueDenseInternal(ctx, index, desc.AsDataDescriptor()->value(), desc.AsDataDescriptor()->IsValueAbsent());
        return true;
      }
      // fall through to sparse array
    } else {
      if (IsAbsentDescriptor(desc)) {
        if (index < elements_.vector.size() && !elements_.vector[index].IsEmpty()) {
          if (!desc.IsValueAbsent()) {
            elements_.vector[index] = desc.AsDataDescriptor()->value();
          }
          return true;
        }
      }

      if (index < IndexedElements::kMaxVectorSize) {
        // purge vector
        elements_.MakeSparse();
      }
    }
  }

  SparseArrayMap* sparse = elements_.EnsureMap();
  std::pair<SparseArrayMap::Entry*, bool> pair =
      sparse->LookupWithFound(index, IsExtensible());
  SparseArrayMap::Entry* entry = pair.first;

  if (!entry) {
    // No entry is found & object is not extensible
    if (throwable) {
      e->Report(Error::Type, "object not extensible");\
    }
    return false;
  }

  if (pair.second) {
    // Entry is found.
    StoredSlot merge(entry->second);
    bool returned = false;
    if (merge.IsDefineOwnPropertyAccepted(desc, throwable, &returned, e)) {
      merge.Merge(ctx, desc);
      entry->second = merge;
    }
    return returned;
  }

  assert(!pair.second);

  if (!map()->IsIndexed()) {
    set_map(map()->ChangeIndexedTransition(ctx));
  }
  if (index >= elements_.length()) {
    elements_.set_length(index + 1);
  }
  entry->second = StoredSlot(ctx, desc);
  return true;
}

inline JSObject* JSObject::New(Context* ctx) {
  JSObject* const obj = NewPlain(ctx, ctx->global_data()->empty_object_map());
  obj->set_cls(JSObject::GetClass());
  return obj;
}

inline JSObject* JSObject::New(Context* ctx, Map* map) {
  JSObject* const obj = NewPlain(ctx, map);
  obj->set_cls(JSObject::GetClass());
  return obj;
}

inline JSObject* JSObject::NewPlain(Context* ctx, Map* map) {
  return new JSObject(map);
}

inline void JSObject::MapTransitionWithReallocation(
    JSObject* base, JSVal src, Map* transit, uint32_t offset) {
  base->set_map(transit);
  base->slots_.resize(transit->GetSlotsSize(), JSEmpty);
  base->Direct(offset) = src;
}

inline void JSCell::MarkChildren(radio::Core* core) {
  core->MarkCell(map_);
}

inline void JSObject::MarkChildren(radio::Core* core) {
  JSCell::MarkChildren(core);
  std::for_each(slots_.begin(), slots_.end(), radio::Core::Marker(core));
  if (elements_.dense()) {
    std::for_each(elements_.vector.begin(), elements_.vector.end(), radio::Core::Marker(core));
  }
  if (elements_.map) {
    for (SparseArrayMap::const_iterator it = elements_.map->begin(),
         last = elements_.map->end(); it != last; ++it) {
      core->MarkValue(it->second.value());
    }
  }
}

inline Map* JSCell::FlattenMap() const {
  // make map transitable
  map()->Flatten();
  return map();
}

inline JSObject* JSCell::prototype() const { return map()->prototype(); }

inline void JSObject::ChangeExtensible(Context* ctx, bool val) {
  if (val) {
    flags_ |= kFlagExtensible;
  } else {
    flags_ &= ~kFlagExtensible;
  }
  set_map(map()->ChangeExtensibleTransition(ctx));
  elements_.MakeSparse();
}


} }  // namespace iv::lv5
#endif  // IV_LV5_JSOBJECT_H_
