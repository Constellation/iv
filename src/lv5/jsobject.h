#ifndef _IV_LV5_JSOBJECT_H_
#define _IV_LV5_JSOBJECT_H_
#include <vector>
#include <gc/gc_cpp.h>
#include "ast.h"
#include "lv5/gc_template.h"
#include "lv5/property.h"
#include "lv5/hint.h"
#include "lv5/symbol.h"

namespace iv {
namespace lv5 {

class JSVal;
class JSFunction;
class Callable;
class Context;
class Error;
class JSString;

class JSObject : public gc {
 public:
  enum EnumerationMode {
    kExcludeNotEnumerable,
    kIncludeNotEnumerable
  };
  typedef GCHashMap<Symbol, PropertyDescriptor>::type Properties;

  JSObject();
  JSObject(JSObject* proto, Symbol class_name, bool extensible);

  virtual JSVal DefaultValue(Context* ctx,
                             Hint::Object hint, Error* res);
  virtual JSVal Get(Context* ctx,
                    Symbol name, Error* res);
  virtual JSVal GetWithIndex(
      Context* context, uint32_t index, Error* res);
  virtual PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const;
  virtual PropertyDescriptor GetOwnPropertyWithIndex(Context* ctx,
                                                     uint32_t index) const;
  virtual PropertyDescriptor GetProperty(Context* ctx, Symbol name) const;
  virtual PropertyDescriptor GetPropertyWithIndex(Context* ctx,
                                                  uint32_t index) const;
  virtual bool CanPut(Context* ctx, Symbol name) const;
  virtual bool CanPutWithIndex(Context* ctx,
                               uint32_t index) const;
  virtual void Put(Context* context, Symbol name,
                   const JSVal& val, bool th, Error* res);
  virtual void PutWithIndex(Context* context, uint32_t index,
                            const JSVal& val, bool th, Error* res);
  virtual bool HasProperty(Context* ctx, Symbol name) const;
  virtual bool HasPropertyWithIndex(Context* ctx, uint32_t index) const;
  virtual bool Delete(Context* ctx, Symbol name, bool th, Error* res);
  virtual bool DeleteWithIndex(Context* ctx, uint32_t index,
                               bool th, Error* res);
  virtual bool DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th,
                                 Error* res);
  virtual bool DefineOwnPropertyWithIndex(Context* ctx,
                                          uint32_t index,
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
  Symbol class_name() const {
    return class_name_;
  }
  void set_class_name(Symbol cls) {
    class_name_ = cls;
  }

  static JSObject* New(Context* ctx);
  static JSObject* NewPlain(Context* ctx);

 protected:
  JSObject* prototype_;
  Symbol class_name_;
  bool extensible_;
  Properties table_;
};

class JSStringObject : public JSObject {
 public:
  JSStringObject(Context* ctx, JSString* value);
  static JSStringObject* New(Context* ctx, JSString* str);
  static JSStringObject* NewPlain(Context* ctx);
  JSString* value() const {
    return value_;
  }
 private:
  JSString* value_;
};

class JSNumberObject : public JSObject {
 public:
  explicit JSNumberObject(const double& value) : value_(value) { }
  static JSNumberObject* New(Context* ctx, const double& value);
  static JSNumberObject* NewPlain(Context* ctx, const double& value);
  const double& value() const {
    return value_;
  }
 private:
  double value_;
};

class JSBooleanObject : public JSObject {
 public:
  explicit JSBooleanObject(bool value) : value_(value) { }
  static JSBooleanObject* NewPlain(Context* ctx, bool value);
  static JSBooleanObject* New(Context* ctx, bool value);
  bool value() const {
    return value_;
  }
 private:
  bool value_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSOBJECT_H_
