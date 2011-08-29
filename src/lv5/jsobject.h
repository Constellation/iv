#ifndef _IV_LV5_JSOBJECT_H_
#define _IV_LV5_JSOBJECT_H_
#include <vector>
#include <gc/gc_cpp.h>
#include "ast.h"
#include "lv5/gc_template.h"
#include "lv5/property.h"
#include "lv5/hint.h"
#include "lv5/symbol.h"
#include "lv5/heap_object.h"
#include "lv5/class.h"
namespace iv {
namespace lv5 {

class Map;
class Slot;
class JSVal;
class JSFunction;
class Callable;
class Context;
class Error;
class JSString;

class JSObject : public HeapObject {
 public:
  enum EnumerationMode {
    kExcludeNotEnumerable,
    kIncludeNotEnumerable
  };
  typedef GCHashMap<Symbol, PropertyDescriptor>::type Properties;

  JSObject(Map* map);
  JSObject(Map* map, JSObject* proto, Class* cls, bool extensible);

  virtual ~JSObject() { }

  virtual JSVal DefaultValue(Context* ctx,
                             Hint::Object hint, Error* e);

  virtual JSVal Get(Context* ctx, Symbol name, Error* e);

  // this is not virtual function
  // if you handle it, override GetOwnPropertySlot
  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;

  virtual bool GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const;

  virtual PropertyDescriptor GetProperty(Context* ctx, Symbol name) const;

  virtual bool CanPut(Context* ctx, Symbol name) const;

  virtual void Put(Context* context, Symbol name,
                   const JSVal& val, bool th, Error* e);

  virtual bool HasProperty(Context* ctx, Symbol name) const;

  virtual bool Delete(Context* ctx, Symbol name, bool th, Error* e);

  virtual bool DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th, Error* e);

  virtual void GetPropertyNames(Context* ctx,
                                std::vector<Symbol>* vec, EnumerationMode mode) const;

  virtual void GetOwnPropertyNames(Context* ctx,
                                   std::vector<Symbol>* vec,
                                   EnumerationMode mode) const;

  JSVal GetFromDescriptor(Context* ctx,
                          const PropertyDescriptor& desc, Error* e);

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

  static const Class* GetClass() {
    static const Class cls = {
      "Object",
      Class::Object
    };
    return &cls;
  }

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

 private:

  const Class* cls_;
  JSObject* prototype_;
  bool extensible_;
  Map* map_;
  GCVector<PropertyDescriptor>::type slots_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSOBJECT_H_
