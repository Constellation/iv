#ifndef IV_LV5_JSOBJECT_FWD_H_
#define IV_LV5_JSOBJECT_FWD_H_
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/property_fwd.h>
#include <iv/lv5/property_names_collector.h>
#include <iv/lv5/hint.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/class.h>
#include <iv/lv5/error.h>
#include <iv/lv5/storage.h>
#include <iv/lv5/indexed_elements.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/core.h>
#include <iv/lv5/jscell.h>
namespace iv {
namespace lv5 {

class Map;
class Slot;
class JSFunction;
class Context;
class Error;

class JSObject : public JSCell {
 public:
  IV_LV5_DEFINE_JSCLASS(JSObject, Object)

  // structure for normal property (hash map is defined in Map)
  typedef FixedStorage<JSVal> Slots;

  // structures for indexed elements
  typedef IndexedElements::SparseArrayMap SparseArrayMap;
  typedef IndexedElements::DenseArrayVector DenseArrayVector;

  static const uint32_t kFlagExtensible = 0x1;
  static const uint32_t kFlagCallable = 0x2;
  static const uint32_t kFlagTuple = 0x4;

  virtual ~JSObject() { }

  // implementation is in map.h
  bool HasIndexedProperty() const;

  bool CanPut(Context* ctx, Symbol name, Slot* slot) const;
  bool CanPutNonIndexed(Context* ctx, Symbol name, Slot* slot) const;
  bool CanPutIndexed(Context* ctx, uint32_t index, Slot* slot) const;

  // generic interfaces in ECMA262 and more
  JSVal Get(Context* ctx, Symbol name, Error* e);
  PropertyDescriptor GetProperty(Context* ctx, Symbol name) const;
  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;
  void Put(Context* ctx, Symbol name, JSVal val, bool throwable, Error* e);
  bool DefineOwnProperty(Context* ctx, Symbol name, const PropertyDescriptor& desc, bool throwable, Error* e);
  bool HasProperty(Context* ctx, Symbol name) const;
  bool HasOwnProperty(Context* ctx, Symbol name) const;

  // simple wrappers
  JSVal GetSlot(Context* ctx, Symbol name, Slot* slot, Error* e);
  JSVal GetNonIndexedSlot(Context* ctx, Symbol name, Slot* slot, Error* e);
  JSVal GetIndexedSlot(Context* ctx, uint32_t index, Slot* slot, Error* e);

  bool GetPropertySlot(Context* ctx, Symbol name, Slot* slot) const;
  bool GetNonIndexedPropertySlot(Context* ctx, Symbol name, Slot* slot) const;
  bool GetIndexedPropertySlot(Context* ctx, uint32_t index, Slot* slot) const;

  bool GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const;
  bool GetOwnNonIndexedPropertySlot(Context* ctx, Symbol name, Slot* slot) const;
  bool GetOwnIndexedPropertySlot(Context* ctx, uint32_t index, Slot* slot) const;

  void PutSlot(Context* ctx, Symbol name, JSVal val, Slot* slot, bool throwable, Error* e);
  void PutNonIndexedSlot(Context* ctx, Symbol name, JSVal val, Slot* slot, bool throwable, Error* e);
  void PutIndexedSlot(Context* ctx, uint32_t index, JSVal val, Slot* slot, bool throwable, Error* e);

  bool Delete(Context* ctx, Symbol name, bool throwable, Error* e);
  bool DeleteNonIndexed(Context* ctx, Symbol name, bool throwable, Error* e);
  bool DeleteIndexed(Context* ctx, uint32_t index, bool throwable, Error* e);

  bool DefineOwnPropertySlot(Context* ctx, Symbol name, const PropertyDescriptor& desc, Slot* slot, bool throwable, Error* e);
  bool DefineOwnNonIndexedPropertySlot(Context* ctx, Symbol name, const PropertyDescriptor& desc, Slot* slot, bool throwable, Error* e);
  bool DefineOwnIndexedPropertySlot(Context* ctx, uint32_t index, const PropertyDescriptor& desc, Slot* slot, bool throwable, Error* e);

  void GetPropertyNames(Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode) const;
  void GetOwnPropertyNames(Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode) const;
  JSVal DefaultValue(Context* ctx, Hint::Object hint, Error* e);

  // override them
  IV_LV5_INTERNAL_METHOD JSVal GetNonIndexedSlotMethod(JSObject* obj, Context* ctx, Symbol name, Slot* slot, Error* e);
  IV_LV5_INTERNAL_METHOD JSVal GetIndexedSlotMethod(JSObject* obj, Context* ctx, uint32_t index, Slot* slot, Error* e);
  IV_LV5_INTERNAL_METHOD bool GetNonIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, Symbol name, Slot* slot);
  IV_LV5_INTERNAL_METHOD bool GetIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, uint32_t index, Slot* slot);
  IV_LV5_INTERNAL_METHOD bool GetOwnNonIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, Symbol name, Slot* slot);
  IV_LV5_INTERNAL_METHOD bool GetOwnIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, uint32_t index, Slot* slot);
  IV_LV5_INTERNAL_METHOD void PutNonIndexedSlotMethod(JSObject* obj, Context* ctx, Symbol name, JSVal val, Slot* slot, bool throwable, Error* e);
  IV_LV5_INTERNAL_METHOD void PutIndexedSlotMethod(JSObject* obj, Context* ctx, uint32_t index, JSVal val, Slot* slot, bool throwable, Error* e);
  IV_LV5_INTERNAL_METHOD bool DeleteNonIndexedMethod(JSObject* obj, Context* ctx, Symbol name, bool throwable, Error* e);
  IV_LV5_INTERNAL_METHOD bool DeleteIndexedMethod(JSObject* obj, Context* ctx, uint32_t index, bool throwable, Error* e);
  IV_LV5_INTERNAL_METHOD bool DefineOwnNonIndexedPropertySlotMethod(JSObject* obj, Context* ctx, Symbol name, const PropertyDescriptor& desc, Slot* slot, bool throwable, Error* e);
  IV_LV5_INTERNAL_METHOD bool DefineOwnIndexedPropertySlotMethod(JSObject* obj, Context* ctx, uint32_t index, const PropertyDescriptor& desc, Slot* slot, bool throwable, Error* e);
  IV_LV5_INTERNAL_METHOD void GetPropertyNamesMethod(const JSObject* obj, Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode);
  IV_LV5_INTERNAL_METHOD void GetOwnPropertyNamesMethod(const JSObject* obj, Context* ctx, PropertyNamesCollector* collector, EnumerationMode mode);
  IV_LV5_INTERNAL_METHOD JSVal DefaultValueMethod(JSObject* obj, Context* ctx, Hint::Object hint, Error* e);

  bool DefineOwnIndexedPropertyInternal(Context* ctx, uint32_t index,
                                        const PropertyDescriptor& desc,
                                        bool throwable, Error* e);
  void DefineOwnIndexedValueDenseInternal(Context* ctx, uint32_t index, JSVal value, bool absent);
  bool DeleteIndexedInternal(Context* ctx, uint32_t inex, bool throwable, Error* e);

  virtual bool IsNativeObject() const { return true; }

  bool Freeze(Context* ctx, Error* e) {
    PropertyNamesCollector collector;
    GetOwnPropertyNames(ctx, &collector, INCLUDE_NOT_ENUMERABLE);
    for (PropertyNamesCollector::Names::const_iterator
         it = collector.names().begin(),
         last = collector.names().end();
         it != last; ++it) {
      const Symbol sym = *it;
      PropertyDescriptor desc = GetOwnProperty(ctx, sym);
      if (desc.IsData()) {
        desc.AsDataDescriptor()->set_writable(false);
      }
      if (desc.IsConfigurable()) {
        desc.set_configurable(false);
      }
      DefineOwnProperty(ctx, sym, desc, true, IV_LV5_ERROR_WITH(e, false));
    }
    ChangeExtensible(ctx, false);
    return true;
  }

  bool Seal(Context* ctx, Error* e) {
    PropertyNamesCollector collector;
    GetOwnPropertyNames(ctx, &collector, INCLUDE_NOT_ENUMERABLE);
    for (PropertyNamesCollector::Names::const_iterator
         it = collector.names().begin(),
         last = collector.names().end();
         it != last; ++it) {
      const Symbol sym = *it;
      PropertyDescriptor desc = GetOwnProperty(ctx, sym);
      if (desc.IsConfigurable()) {
        desc.set_configurable(false);
      }
      DefineOwnProperty(ctx, sym, desc, true, IV_LV5_ERROR_WITH(e, false));
    }
    ChangeExtensible(ctx, false);
    return true;
  }

  bool IsCallable() const { return flags_ & kFlagCallable; }

  void set_callable(bool val) {
    if (val) {
      flags_ |= kFlagCallable;
    } else {
      flags_ &= ~kFlagCallable;
    }
  }

  bool IsExtensible() const { return flags_ & kFlagExtensible; }

  void ChangeExtensible(Context* ctx, bool val);

  static JSObject* New(Context* ctx);

  static JSObject* New(Context* ctx, Map* map);

  static JSObject* NewPlain(Context* ctx, Map* map);

  inline const JSVal& Direct(std::size_t n) const {
    assert(slots_.size() > n);
    return slots_[n];
  }

  inline JSVal& Direct(std::size_t n) {
    assert(slots_.size() > n);
    return slots_[n];
  }

  virtual void MarkChildren(radio::Core* core);

  const MethodTable* method() const { return &cls()->method; }

  void ChangePrototype(Context* ctx, JSObject* proto);

  static std::size_t SlotsOffset() { return IV_OFFSETOF(JSObject, slots_); }
  static std::size_t ElementsOffset() { return IV_OFFSETOF(JSObject, elements_); }

  static void MapTransitionWithReallocation(
      JSObject* base, JSVal src, Map* transit, uint32_t offset);

  void MakeTuple() { flags_ |= kFlagTuple; }
  bool IsTuple() const { return flags_ & kFlagTuple; }

 protected:
  explicit JSObject(Map* map);
  explicit JSObject(const JSObject& obj);
  JSObject(Map* map, const Class* cls);

  Slots slots_;
  IndexedElements elements_;
  uint32_t flags_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSOBJECT_FWD_H_
