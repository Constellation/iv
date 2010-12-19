#ifndef _IV_LV5_JSOBJECT_H_
#define _IV_LV5_JSOBJECT_H_
#include <vector>
#include <gc/gc_cpp.h>
#include "ast.h"
#include "property.h"
#include "hint.h"
#include "jsstring.h"
#include "gc_template.h"
#include "symbol.h"

namespace iv {
namespace lv5 {

class JSVal;
class JSFunction;
class PropertyDescriptor;
class Callable;
class JSNumberObject;
class JSStringObject;
class JSBooleanObject;
class Context;
class Error;

class JSObject : public gc {
 public:
  typedef GCHashMap<Symbol, PropertyDescriptor>::type Properties;

  JSObject();
  JSObject(JSObject* proto, Symbol class_name, bool extensible);

  virtual JSVal DefaultValue(Context* context,
                             Hint::Object hint, Error* res);
  virtual JSVal Get(Context* context,
                    Symbol name, Error* res);
  virtual PropertyDescriptor GetOwnProperty(Symbol name) const;
  virtual PropertyDescriptor GetProperty(Symbol name) const;
  virtual bool CanPut(Symbol name) const;
  virtual void Put(Context* context, Symbol name,
                   const JSVal& val, bool th, Error* res);
  virtual bool HasProperty(Symbol name) const;
  virtual bool Delete(Symbol name, bool th, Error* res);
  virtual bool DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th,
                                 Error* res);
  void GetPropertyNames(std::vector<Symbol>* vec) const;
  virtual void GetOwnPropertyNames(std::vector<Symbol>* vec) const;
  virtual bool IsCallable() const {
    return false;
  }
  virtual JSFunction* AsCallable() {
    return NULL;
  }
  virtual JSNumberObject* AsNumberObject() {
    return NULL;
  }
  virtual JSBooleanObject* AsBooleanObject() {
    return NULL;
  }
  virtual JSStringObject* AsStringObject() {
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
  const Properties& table() const {
    return table_;
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
  explicit JSStringObject(JSString* value) : value_(value) { }
  static JSStringObject* New(Context* ctx, JSString* str);
  static JSStringObject* NewPlain(Context* ctx);
  JSStringObject* AsStringObject() {
    return this;
  }
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
  JSNumberObject* AsNumberObject() {
    return this;
  }
  const double& value() const {
    return value_;
  }
 private:
  double value_;
};

class JSBooleanObject : public JSObject {
 public:
  explicit JSBooleanObject(bool value);
  static JSBooleanObject* NewPlain(Context* ctx, bool value);
  static JSBooleanObject* New(Context* ctx, bool value);
  JSBooleanObject* AsBooleanObject() {
    return this;
  }
  bool value() const {
    return value_;
  }
 private:
  bool value_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSOBJECT_H_
