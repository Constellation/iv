#ifndef IV_LV5_JSOBJECT_H_
#define IV_LV5_JSOBJECT_H_
#include <vector>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/property_fwd.h>
#include <iv/lv5/property_names_collector.h>
#include <iv/lv5/hint.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/class.h>
#include <iv/lv5/storage.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/core_fwd.h>
#include <iv/lv5/breaker/fwd.h>
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

  friend class breaker::Compiler;
  friend class breaker::MonoIC;

  typedef Storage<JSVal> Slots;

  enum EnumerationMode {
    EXCLUDE_NOT_ENUMERABLE,
    INCLUDE_NOT_ENUMERABLE
  };

  virtual ~JSObject() { }

  virtual JSVal DefaultValue(Context* ctx, Hint::Object hint, Error* e);

  // if you handle it, override
  //   GetSlot
  //   GetPropertySlot
  //   GetOwnPropertySlot
  // instead of
  //   Get
  //   GetProperty
  //   GetOwnProperty
  JSVal Get(Context* ctx, Symbol name, Error* e);
  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;
  PropertyDescriptor GetProperty(Context* ctx, Symbol name) const;
  virtual JSVal GetSlot(Context* ctx, Symbol name, Slot* slot, Error* e);
  virtual bool GetPropertySlot(Context* ctx, Symbol name, Slot* slot) const;
  virtual bool GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const;

  bool CanPut(Context* ctx, Symbol name) const;

  virtual void Put(Context* context, Symbol name,
                   JSVal val, bool th, Error* e);

  virtual bool HasProperty(Context* ctx, Symbol name) const;

  virtual bool Delete(Context* ctx, Symbol name, bool th, Error* e);

  bool DeleteDirect(Context* ctx, Symbol name, bool th, Error* e);

  virtual bool DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th, Error* e);

  virtual void GetPropertyNames(Context* ctx,
                                PropertyNamesCollector* collector,
                                EnumerationMode mode) const;

  virtual void GetOwnPropertyNames(Context* ctx,
                                   PropertyNamesCollector* collector,
                                   EnumerationMode mode) const;

  virtual bool IsCallable() const {
    return false;
  }

  virtual JSFunction* AsCallable() {
    return NULL;
  }

  virtual bool IsNativeObject() const {
    return true;
  }

  bool IsExtensible() const {
    return extensible_;
  }

  void set_extensible(bool val) {
    extensible_ = val;
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

 protected:
  explicit JSObject(Map* map);

  JSObject(Map* map, JSObject* proto, Class* cls, bool extensible);

  const Class* cls_;
  JSObject* prototype_;
  bool extensible_;
  Map* map_;
  Slots slots_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSOBJECT_H_
