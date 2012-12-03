#ifndef IV_LV5_JSOBJECT_INTERFACE_H_
#define IV_LV5_JSOBJECT_INTERFACE_H_
namespace iv {
namespace lv5 {

inline bool JSObject::HasIndexedProperty() const {
  const JSObject* obj = this;
  do {
    if (obj->map()->IsIndexed()) {
      return true;
    }
    obj = obj->prototype();
  } while (obj);
  return false;
}

// GetSlot
inline JSVal JSObject::GetSlot(Context* ctx, Symbol name, Slot* slot, Error* e) {
  if (symbol::IsArrayIndexSymbol(name)) {
    return JSObject::GetIndexedSlot(ctx, symbol::GetIndexFromSymbol(name), slot, e);
  }
  return JSObject::GetNonIndexedSlot(ctx, name, slot, e);
}

inline JSVal JSObject::GetNonIndexedSlot(Context* ctx, Symbol name, Slot* slot, Error* e) {
  return method()->GetNonIndexedSlot(this, ctx, name, slot, e);
}

inline JSVal JSObject::GetIndexedSlot(Context* ctx, uint32_t index, Slot* slot, Error* e) {
  return method()->GetIndexedSlot(this, ctx, index, slot, e);
}

// GetPropertySlot
inline bool JSObject::GetPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
  if (symbol::IsArrayIndexSymbol(name)) {
    return JSObject::GetIndexedPropertySlot(ctx, symbol::GetIndexFromSymbol(name), slot);
  }
  return JSObject::GetNonIndexedPropertySlot(ctx, name, slot);
}

inline bool JSObject::GetNonIndexedPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
  return method()->GetNonIndexedPropertySlot(this, ctx, name, slot);
}

inline bool JSObject::GetIndexedPropertySlot(Context* ctx, uint32_t index, Slot* slot) const {
  return map()->IsIndexed() && method()->GetIndexedPropertySlot(this, ctx, index, slot);
}

// GetOwnPropertySlot
inline bool JSObject::GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
  if (symbol::IsArrayIndexSymbol(name)) {
    return JSObject::GetOwnIndexedPropertySlot(ctx, symbol::GetIndexFromSymbol(name), slot);
  }
  return JSObject::GetOwnNonIndexedPropertySlot(ctx, name, slot);
}

inline bool JSObject::GetOwnNonIndexedPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
  return method()->GetOwnNonIndexedPropertySlot(this, ctx, name, slot);
}

inline bool JSObject::GetOwnIndexedPropertySlot(Context* ctx, uint32_t index, Slot* slot) const {
  return map()->IsIndexed() && method()->GetOwnIndexedPropertySlot(this, ctx, index, slot);
}

// PutSlot
inline void JSObject::PutSlot(Context* ctx, Symbol name, JSVal val, Slot* slot, bool throwable, Error* e) {
  if (symbol::IsArrayIndexSymbol(name)) {
    return JSObject::PutIndexedSlot(ctx, symbol::GetIndexFromSymbol(name), val, slot, throwable, e);
  }
  return JSObject::PutNonIndexedSlot(ctx, name, val, slot, throwable, e);
}

inline void JSObject::PutNonIndexedSlot(Context* ctx, Symbol name, JSVal val, Slot* slot, bool throwable, Error* e) {
  method()->PutNonIndexedSlot(this, ctx, name, val, slot, throwable, e);
}

inline void JSObject::PutIndexedSlot(Context* ctx, uint32_t index, JSVal val, Slot* slot, bool throwable, Error* e) {
  method()->PutIndexedSlot(this, ctx, index, val, slot, throwable, e);
}

// Delete
inline bool JSObject::Delete(Context* ctx, Symbol name, bool throwable, Error* e) {
  if (symbol::IsArrayIndexSymbol(name)) {
    return JSObject::DeleteIndexed(ctx, symbol::GetIndexFromSymbol(name), throwable, e);
  }
  return JSObject::DeleteNonIndexed(ctx, name, throwable, e);
}

inline bool JSObject::DeleteNonIndexed(Context* ctx, Symbol name, bool throwable, Error* e) {
  return method()->DeleteNonIndexed(this, ctx, name, throwable, e);
}

inline bool JSObject::DeleteIndexed(Context* ctx, uint32_t index, bool throwable, Error* e) {
  return method()->DeleteIndexed(this, ctx, index, throwable, e);
}

// DefineOwnPropertySlot
inline bool JSObject::DefineOwnPropertySlot(Context* ctx, Symbol name, const PropertyDescriptor& desc, Slot* slot, bool throwable, Error* e) {
  if (symbol::IsArrayIndexSymbol(name)) {
    return JSObject::DefineOwnIndexedPropertySlot(ctx, symbol::GetIndexFromSymbol(name), desc, slot, throwable, e);
  }
  return JSObject::DefineOwnNonIndexedPropertySlot(ctx, name, desc, slot, throwable, e);
}

inline bool JSObject::DefineOwnNonIndexedPropertySlot(Context* ctx, Symbol name, const PropertyDescriptor& desc, Slot* slot, bool throwable, Error* e) {
  return method()->DefineOwnNonIndexedPropertySlot(this, ctx, name, desc, slot, throwable, e);
}

inline bool JSObject::DefineOwnIndexedPropertySlot(Context* ctx, uint32_t index, const PropertyDescriptor& desc, Slot* slot, bool throwable, Error* e) {
  return method()->DefineOwnIndexedPropertySlot(this, ctx, index, desc, slot, throwable, e);
}

inline void JSObject::GetPropertyNames(Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode) const {
  return method()->GetPropertyNames(this, ctx, collector, mode);
}

inline void JSObject::GetOwnPropertyNames(Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode) const {
  return method()->GetOwnPropertyNames(this, ctx, collector, mode);
}

inline JSVal JSObject::DefaultValue(Context* ctx, Hint::Object hint, Error* e) {
  return method()->DefaultValue(this, ctx, hint, e);
}

// CanPut
inline bool JSObject::CanPut(Context* ctx, Symbol name, Slot* slot) const {
  if (symbol::IsArrayIndexSymbol(name)) {
    return CanPutIndexed(ctx, symbol::GetIndexFromSymbol(name), slot);
  }
  return CanPutNonIndexed(ctx, name, slot);
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSOBJECT_INTERFACE_H_
