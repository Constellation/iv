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

  enum EnumerationMode {
    EXCLUDE_NOT_ENUMERABLE,
    INCLUDE_NOT_ENUMERABLE
  };
  typedef GCHashMap<Symbol, PropertyDescriptor>::type Properties;

  virtual ~JSObject() { }

  virtual JSVal DefaultValue(Context* ctx, Hint::Object hint, Error* e);

  virtual JSVal Get(Context* ctx, Symbol name, Error* e);

  // if you handle it, override GetOwnPropertySlot
  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;

  // if you handle it, override GetPropertySlot
  PropertyDescriptor GetProperty(Context* ctx, Symbol name) const;

  // hook these functions

  virtual bool GetPropertySlot(Context* ctx, Symbol name, Slot* slot) const;

  virtual bool GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const;

  bool CanPut(Context* ctx, Symbol name) const;

  virtual void Put(Context* context, Symbol name,
                   const JSVal& val, bool th, Error* e);

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

  JSVal GetBySlotOffset(Context* ctx, std::size_t n, Error* e);

  void PutToSlotOffset(Context* ctx, std::size_t offset,
                       const JSVal& val, bool th, Error* e);

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
    return cls_->type == CLS;
  }

  bool IsClass(Class::JSClassType cls) const {
    return cls_->type == cls;
  }

  static JSObject* New(Context* ctx);

  static JSObject* New(Context* ctx, Map* map);

  static JSObject* NewPlain(Context* ctx);

  static JSObject* NewPlain(Context* ctx, Map* map);

  Map* FlattenMap() const;

  const PropertyDescriptor& GetSlot(std::size_t n) const {
    assert(slots_.size() > n);
    return slots_[n];
  }

  PropertyDescriptor& GetSlot(std::size_t n) {
    assert(slots_.size() > n);
    return slots_[n];
  }

  Map* map() const {
    return map_;
  }

  void MarkChildren(radio::Core* core);

 protected:
  explicit JSObject(Map* map);

  JSObject(Map* map, JSObject* proto, Class* cls, bool extensible);

  const Class* cls_;
  JSObject* prototype_;
  bool extensible_;
  Map* map_;
  GCVector<PropertyDescriptor>::type slots_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSOBJECT_H_
