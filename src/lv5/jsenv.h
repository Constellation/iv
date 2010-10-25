#ifndef _IV_LV5_JSENV_H_
#define _IV_LV5_JSENV_H_
#include <gc/gc_cpp.h>
#include "gc-template.h"
#include "jsval.h"
#include "symbol.h"

namespace iv {
namespace lv5 {

class JSDeclEnv;
class JSObjectEnv;
class Context;
class JSEnv : public gc {
 public:
  virtual bool HasBinding(Symbol name) const = 0;
  virtual bool DeleteBinding(Symbol name) = 0;
  virtual void CreateMutableBinding(Context* ctx, Symbol name, bool del) = 0;
  virtual void SetMutableBinding(Context* ctx,
                                 Symbol name,
                                 const JSVal& val,
                                 bool strict, Error* res) = 0;
  virtual JSVal GetBindingValue(Context* ctx, Symbol name,
                                bool strict, Error* res) const = 0;
  virtual JSVal ImplicitThisValue() const = 0;
  virtual JSDeclEnv* AsJSDeclEnv() = 0;
  virtual JSObjectEnv* AsJSObjectEnv() = 0;
  inline JSEnv* outer() const {
    return outer_;
  }
 protected:
  explicit JSEnv(JSEnv* outer) : outer_(outer) { }
  JSEnv* outer_;
};

class JSDeclEnv : public JSEnv {
 public:
  enum RecordType {
    IM_INITIALIZED = 1,
    IM_UNINITIALIZED = 2,
    MUTABLE = 4,
    DELETABLE = 8
  };
  typedef GCHashMap<Symbol, std::pair<int, JSVal> >::type Record;
  explicit JSDeclEnv(JSEnv* outer)
    : JSEnv(outer),
      record_() {
  }
  bool HasBinding(Symbol name) const;
  bool DeleteBinding(Symbol name);
  void CreateMutableBinding(Context* ctx, Symbol name, bool del);
  void SetMutableBinding(Context* ctx,
                         Symbol name,
                         const JSVal& val,
                         bool strict, Error* res);
  JSVal GetBindingValue(Context* ctx, Symbol name,
                        bool strict, Error* res) const;
  JSVal ImplicitThisValue() const;
  void CreateImmutableBinding(Symbol name);
  void InitializeImmutableBinding(Symbol name, const JSVal& val);

  JSDeclEnv* AsJSDeclEnv() {
    return this;
  }

  JSObjectEnv* AsJSObjectEnv() {
    return NULL;
  }

  Record& record() {
    return record_;
  }

  static JSDeclEnv* New(Context* ctx, JSEnv* outer);
 private:
  Record record_;
};


class JSObjectEnv : public JSEnv {
 public:
  explicit JSObjectEnv(JSEnv* outer, JSObject* rec)
    : JSEnv(outer),
      record_(rec),
      provide_this_(false) {
  }
  bool HasBinding(Symbol name) const;
  bool DeleteBinding(Symbol name);
  void CreateMutableBinding(Context* ctx, Symbol name, bool del);
  void SetMutableBinding(Context* ctx,
                         Symbol name,
                         const JSVal& val,
                         bool strict, Error* res);
  JSVal GetBindingValue(Context* ctx, Symbol name,
                        bool strict, Error* res) const;
  JSVal ImplicitThisValue() const;

  JSDeclEnv* AsJSDeclEnv() {
    return NULL;
  }

  JSObjectEnv* AsJSObjectEnv() {
    return this;
  }

  JSObject* record() {
    return record_;
  }
  bool provie_this() {
    return provide_this_;
  }
  void set_provide_this(bool val) {
    provide_this_ = val;
  }

  static JSObjectEnv* New(Context* ctx, JSEnv* outer, JSObject* rec);
 private:
  JSObject* record_;
  bool provide_this_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSENV_H_
