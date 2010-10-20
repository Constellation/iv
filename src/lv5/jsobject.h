#ifndef _IV_LV5_JSOBJECT_H_
#define _IV_LV5_JSOBJECT_H_
#include <gc/gc_cpp.h>
#include "ast.h"
#include "property.h"
#include "hint.h"
#include "jsstring.h"
#include "jserrorcode.h"
#include "gc-template.h"
#include "symbol.h"

namespace iv {
namespace lv5 {

class JSVal;
class JSFunction;
class PropertyDescriptor;
class Callable;
class Context;

class JSObject : public gc {
 public:
  typedef GCHashMap<Symbol, PropertyDescriptor>::type Properties;

  JSObject();
  JSObject(JSObject* proto, JSString* cls, bool extensible);

  virtual JSVal DefaultValue(Context* context,
                             Hint::Object hint, JSErrorCode::Type* res);
  virtual JSVal Get(Context* context,
                    Symbol name, JSErrorCode::Type* res);
  virtual PropertyDescriptor GetOwnProperty(Symbol name) const;
  virtual PropertyDescriptor GetProperty(Symbol name) const;
  virtual bool CanPut(Symbol name) const;
  virtual void Put(Context* context, Symbol name,
                   const JSVal& val, bool th, JSErrorCode::Type* res);
  virtual bool HasProperty(Symbol name) const;
  virtual bool Delete(Symbol name, bool th, JSErrorCode::Type* res);
  virtual bool DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th,
                                 JSErrorCode::Type* res);
  virtual bool IsCallable() const {
    return false;
  }
  virtual JSFunction* AsCallable() {
    return NULL;
  }

  bool IsExtensible() const {
    return extensible_;
  }
  JSObject* prototype() const {
    return prototype_;
  }
  void set_prototype(JSObject* obj) {
    prototype_ = obj;
  }
  JSString* cls() const {
    return cls_;
  }
  void set_cls(JSString* str) {
    cls_ = str;
  }
  const Properties& table() const {
    return table_;
  }

  static JSObject* New(Context* ctx);
  static JSObject* NewPlain(Context* ctx);

 protected:
  JSObject* prototype_;
  JSString* cls_;
  bool extensible_;
  Properties table_;
};

class JSStringObject : public JSObject {
 public:
  explicit JSStringObject(JSString* value) : value_(value) { }
  static JSStringObject* New(Context* ctx, JSString* str);
 private:
  JSString* value_;
};

class JSNumberObject : public JSObject {
 public:
  explicit JSNumberObject(const double& value) : value_(value) { }
  static JSNumberObject* New(Context* ctx, const double& value);
 private:
  double value_;
};

class JSBooleanObject : public JSObject {
 public:
  explicit JSBooleanObject(bool value);
  static JSBooleanObject* New(Context* ctx, bool value);
 private:
  bool value_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSOBJECT_H_
