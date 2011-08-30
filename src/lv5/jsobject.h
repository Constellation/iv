#ifndef IV_LV5_JSOBJECT_H_
#define IV_LV5_JSOBJECT_H_
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

  JSObject();
  JSObject(JSObject* proto, Class* cls, bool extensible);
  JSObject(Properties* table);

  virtual ~JSObject() { }

  virtual JSVal DefaultValue(Context* ctx,
                             Hint::Object hint, Error* res);

  virtual JSVal Get(Context* ctx,
                    Symbol name, Error* res);

  virtual PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;

  virtual PropertyDescriptor GetProperty(Context* ctx, Symbol name) const;

  virtual bool CanPut(Context* ctx, Symbol name) const;

  virtual void Put(Context* context, Symbol name,
                   const JSVal& val, bool th, Error* res);

  virtual bool HasProperty(Context* ctx, Symbol name) const;

  virtual bool Delete(Context* ctx, Symbol name, bool th, Error* res);

  virtual bool DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th,
                                 Error* res);

  void GetPropertyNames(Context* ctx,
                        std::vector<Symbol>* vec, EnumerationMode mode) const;

  virtual void GetOwnPropertyNames(Context* ctx,
                                   std::vector<Symbol>* vec,
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
    return cls_->type == CLS;
  }

  bool IsClass(Class::JSClassType cls) const {
    return cls_->type == cls;
  }

  static JSObject* New(Context* ctx);
  static JSObject* NewPlain(Context* ctx);

  static const Class* GetClass() {
    static const Class cls = {
      "Object",
      Class::Object
    };
    return &cls;
  }

 private:
  void AllocateTable() {
    if (!table_) {
      table_ = new(GC)Properties();
    }
  }

  const Class* cls_;
  JSObject* prototype_;
  bool extensible_;
  Properties* table_;
};

template<std::size_t N>
class JSObjectWithSlot : public JSObject {
 public:
  static const std::size_t kSlotSize = N;

  std::size_t GetSlotSize() {
    return N;
  }

  JSVal& GetSlot(std::size_t n) {
    assert(n < N);
    return slots_[n];
  }

  const JSVal& GetSlot(std::size_t n) const {
    assert(n < N);
    return slots_[n];
  }

 protected:
  std::array<JSVal, N> slots_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSOBJECT_H_
