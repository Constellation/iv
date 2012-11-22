#ifndef IV_LV5_JSOBJECT_H_
#define IV_LV5_JSOBJECT_H_
#include <vector>
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
  IV_LV5_DEFINE_JSCLASS(Object)
  typedef FixedStorage<JSVal> Slots;

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

  bool DeleteDirect(Context* ctx, Symbol name, bool th, Error* e);

  virtual bool IsNativeObject() const {
    return true;
  }

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
    set_extensible(false);
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
    set_extensible(false);
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

  void set_extensible(bool val) {
    if (val) {
      flags_ |= kFlagExtensible;
    } else {
      flags_ &= ~kFlagExtensible;
    }
  }

  JSObject* prototype() const {
    return prototype_;
  }

  void set_prototype(JSObject* obj) {
    prototype_ = obj;
  }

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

  static JSObject* New(Context* ctx, Map* map, JSObject* prototype);

  static JSObject* NewPlain(Context* ctx);

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

  static std::size_t MapOffset() { return IV_OFFSETOF(JSObject, map_); }
  static std::size_t PrototypeOffset() { return IV_OFFSETOF(JSObject, prototype_); }
  static std::size_t SlotsOffset() { return IV_OFFSETOF(JSObject, slots_); }
  static std::size_t ClassOffset() { return IV_OFFSETOF(JSObject, cls_); }

  static void MapTransitionWithReallocation(
      JSObject* base, JSVal src, Map* transit, uint32_t offset);

 protected:
  explicit JSObject(Map* map);

  JSObject(Map* map, JSObject* proto, Class* cls, bool extensible);

  const Class* cls_;
  Map* map_;
  JSObject* prototype_;
  Slots slots_;
  uint32_t flags_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSOBJECT_H_
