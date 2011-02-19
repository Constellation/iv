#ifndef _IV_LV5_JSENV_H_
#define _IV_LV5_JSENV_H_
#include <cassert>
#include <gc/gc_cpp.h>
#include "lv5/gc_template.h"
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/symbol.h"
#include "lv5/property.h"
#include "lv5/context_utils.h"
#include "lv5/error.h"

namespace iv {
namespace lv5 {

class JSDeclEnv;
class JSObjectEnv;
class Context;

class JSEnv : public gc {
 public:
  virtual bool HasBinding(Context* ctx, Symbol name) const = 0;
  virtual bool DeleteBinding(Context* ctx, Symbol name) = 0;
  virtual void CreateMutableBinding(Context* ctx, Symbol name,
                                    bool del, Error* err) = 0;
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

  bool HasBinding(Context* ctx, Symbol name) const {
    return record_.find(name) != record_.end();
  }

  bool DeleteBinding(Context* ctx, Symbol name) {
    const Record::const_iterator it(record_.find(name));
    if (it == record_.end()) {
      return true;
    }
    if (it->second.first & DELETABLE) {
      record_.erase(it);
      return true;
    } else {
      return false;
    }
  }

  void CreateMutableBinding(Context* ctx, Symbol name, bool del, Error* err) {
    assert(record_.find(name) == record_.end());
    int flag = MUTABLE;
    if (del) {
      flag |= DELETABLE;
    }
    record_[name] = std::make_pair(flag, JSUndefined);
  }

  void SetMutableBinding(Context* ctx,
                         Symbol name,
                         const JSVal& val,
                         bool strict, Error* res) {
    const Record::const_iterator it(record_.find(name));
    assert(it != record_.end());
    if (it->second.first & MUTABLE) {
      record_[name] = std::make_pair(it->second.first, val);
    } else {
      if (strict) {
        res->Report(Error::Type, "mutating immutable binding not allowed");
      }
    }
  }

  JSVal GetBindingValue(Context* ctx, Symbol name,
                        bool strict, Error* res) const {
    const Record::const_iterator it(record_.find(name));
    assert(it != record_.end());
    if (it->second.first & IM_UNINITIALIZED) {
      if (strict) {
        res->Report(Error::Reference,
                    "uninitialized value access not allowed in strict code");
      }
      return JSUndefined;
    } else {
      return it->second.second;
    }
  }

  JSVal GetBindingValue(Symbol name) const {
    const Record::const_iterator it(record_.find(name));
    assert(it != record_.end() && !(it->second.first & IM_UNINITIALIZED));
    return it->second.second;
  }

  JSVal ImplicitThisValue() const {
    return JSUndefined;
  }

  void CreateImmutableBinding(Symbol name) {
    assert(record_.find(name) == record_.end());
    record_[name] = std::make_pair(IM_UNINITIALIZED, JSUndefined);
  }

  void InitializeImmutableBinding(Symbol name, const JSVal& val) {
    assert(record_.find(name) != record_.end() &&
           (record_.find(name)->second.first & IM_UNINITIALIZED));
    record_[name] = std::make_pair(IM_INITIALIZED, val);
  }

  JSDeclEnv* AsJSDeclEnv() {
    return this;
  }

  JSObjectEnv* AsJSObjectEnv() {
    return NULL;
  }

  Record& record() {
    return record_;
  }

  static JSDeclEnv* New(Context* ctx, JSEnv* outer) {
    return new JSDeclEnv(outer);
  }

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

  bool HasBinding(Context* ctx, Symbol name) const {
    return record_->HasProperty(ctx, name);
  }

  bool DeleteBinding(Context* ctx, Symbol name) {
    return record_->Delete(ctx, name, false, NULL);
  }

  void CreateMutableBinding(Context* ctx, Symbol name, bool del, Error* err) {
    assert(!record_->HasProperty(ctx, name));
    int attr = PropertyDescriptor::WRITABLE |
               PropertyDescriptor::ENUMERABLE;
    if (del) {
      attr |= PropertyDescriptor::CONFIGURABLE;
    }
    record_->DefineOwnProperty(
        ctx,
        name,
        DataDescriptor(JSUndefined, attr),
        true,
        err);
  }

  void SetMutableBinding(Context* ctx,
                         Symbol name,
                         const JSVal& val,
                         bool strict, Error* res) {
    record_->Put(ctx, name, val, strict, res);
  }

  JSVal GetBindingValue(Context* ctx, Symbol name,
                        bool strict, Error* res) const {
    const bool value = record_->HasProperty(ctx, name);
    if (!value) {
      if (strict) {
        StringBuilder builder;
        builder.Append('"');
        builder.Append(context::GetSymbolString(ctx, name));
        builder.Append("\" not defined");
        res->Report(Error::Reference,
                    builder.BuildUStringPiece());
      }
      return JSUndefined;
    }
    return record_->Get(ctx, name, res);
  }

  JSVal ImplicitThisValue() const {
    if (provide_this_) {
      return record_;
    } else {
      return JSUndefined;
    }
  }

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

  static JSObjectEnv* New(Context* ctx, JSEnv* outer, JSObject* rec) {
    return new JSObjectEnv(outer, rec);
  }

 private:
  JSObject* record_;
  bool provide_this_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSENV_H_
