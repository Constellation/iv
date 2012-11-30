#ifndef IV_LV5_JSOBJECT_H_
#define IV_LV5_JSOBJECT_H_
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
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {

class Map;
class Slot;
class JSFunction;
class Context;
class Error;

class JSObject : public radio::HeapObject<radio::OBJECT> {
 public:
  IV_LV5_DEFINE_JSCLASS(JSObject, Object)

  // structure for normal property (hash map is defined in Map)
  typedef FixedStorage<JSVal> Slots;

  // structures for indexed elements
  typedef IndexedElements::SparseArrayMap SparseArrayMap;
  typedef IndexedElements::DenseArrayVector DenseArrayVector;

  static const uint32_t kFlagExtensible = 0x1;
  static const uint32_t kFlagCallable = 0x2;

  virtual ~JSObject() { }

  virtual JSVal DefaultValue(Context* ctx, Hint::Object hint, Error* e);

  bool CanPut(Context* ctx, Symbol name, Slot* slot) const;

  // if you would like to handle them, override
  //   GetSlot
  //   GetPropertySlot
  //   GetOwnPropertySlot
  //   PutSlot
  //   DefineOwnPropertySlot
  // instead of
  //   Get
  //   GetProperty
  //   GetOwnProperty
  //   Put
  //   DefineOwnProperty
  JSVal Get(Context* ctx, Symbol name, Error* e);
  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;
  PropertyDescriptor GetProperty(Context* ctx, Symbol name) const;
  bool HasOwnProperty(Context* ctx, Symbol name) const;
  void Put(Context* context, Symbol name, JSVal val, bool th, Error* e);
  bool DefineOwnProperty(Context* ctx,
                         Symbol name,
                         const PropertyDescriptor& desc,
                         bool th, Error* e);

  virtual JSVal GetSlot(Context* ctx, Symbol name, Slot* slot, Error* e);
  virtual bool GetPropertySlot(Context* ctx, Symbol name, Slot* slot) const;
  virtual bool GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const;
  virtual void PutSlot(Context* context, Symbol name, JSVal val, Slot* slot, bool th, Error* e);
  virtual bool HasProperty(Context* ctx, Symbol name) const;
  virtual bool Delete(Context* ctx, Symbol name, bool th, Error* e);
  virtual bool DefineOwnPropertySlot(Context* ctx,
                                     Symbol name,
                                     const PropertyDescriptor& desc,
                                     Slot* slot,
                                     bool th, Error* e);
  virtual void GetPropertyNames(Context* ctx,
                                PropertyNamesCollector* collector,
                                EnumerationMode mode) const;
  virtual void GetOwnPropertyNames(Context* ctx,
                                   PropertyNamesCollector* collector,
                                   EnumerationMode mode) const;

  bool GetOwnIndexedPropertySlotInternal(Context* ctx, uint32_t index, Slot* slot) const;
  bool DefineOwnIndexedPropertyInternal(Context* ctx, uint32_t index,
                                        const PropertyDescriptor& desc,
                                        bool th, Error* e);
  bool DeleteIndexedInternal(Context* ctx, uint32_t inex, bool th, Error* e);

  virtual bool IsNativeObject() const { return true; }

  virtual bool Freeze(Context* ctx, Error* e) {
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

  virtual bool Seal(Context* ctx, Error* e) {
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

  JSObject* prototype() const;

  const Class* cls() const {
    return cls_;
  }

  void set_cls(const Class* cls) {
    cls_ = cls;
  }

  template<Class::JSClassType CLS>
  bool IsClass() const {
    return cls_->type == static_cast<uint32_t>(CLS);
  }

  bool IsClass(Class::JSClassType cls) const {
    return cls_->type == static_cast<uint32_t>(cls);
  }

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

  void MarkChildren(radio::Core* core);

  Map* map() const { return map_; }

  void set_map(Map* map) { map_ = map; }

  Map* FlattenMap() const;

  void ChangePrototype(Context* ctx, JSObject* proto);

  static std::size_t MapOffset() { return IV_OFFSETOF(JSObject, map_); }
  static std::size_t SlotsOffset() { return IV_OFFSETOF(JSObject, slots_); }
  static std::size_t ClassOffset() { return IV_OFFSETOF(JSObject, cls_); }
  static std::size_t ElementsOffset() { return IV_OFFSETOF(JSObject, elements_); }

  static void MapTransitionWithReallocation(
      JSObject* base, JSVal src, Map* transit, uint32_t offset);
 protected:
  explicit JSObject(Map* map);
  explicit JSObject(const JSObject& obj);
  JSObject(Map* map, Class* cls);

  const Class* cls_;
  Map* map_;
  Slots slots_;
  IndexedElements elements_;
  uint32_t flags_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSOBJECT_H_
