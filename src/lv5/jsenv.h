#ifndef IV_LV5_JSENV_H_
#define IV_LV5_JSENV_H_
#include <gc/gc_cpp.h>
#include "notfound.h"
#include "debug.h"
#include "lv5/gc_template.h"
#include "lv5/jsobject.h"
#include "lv5/symbol.h"
#include "lv5/property_fwd.h"
#include "lv5/context_utils.h"
#include "lv5/error.h"
#include "lv5/cell.h"
#include "lv5/jsval_fwd.h"
#include "lv5/string_builder.h"

namespace iv {
namespace lv5 {

class JSDeclEnv;
class JSObjectEnv;
class JSStaticEnv;

class JSEnv : public radio::HeapObject<radio::ENVIRONMENT> {
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
  virtual JSStaticEnv* AsJSStaticEnv() = 0;
  virtual bool IsLookupNeeded() const = 0;
  inline JSEnv* outer() const {
    return outer_;
  }

 protected:
  explicit JSEnv(JSEnv* outer)
    : outer_(outer) {
  }

 private:

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

  struct Entry {
    Entry() : attribute(0), value(JSEmpty) { }
    Entry(int attr) : attribute(attr), value(JSUndefined) { }
    Entry(int attr, const JSVal& val) : attribute(attr), value(val) { }

    int attribute;
    JSVal value;
  };

  typedef GCVector<Entry>::type Record;
  typedef GCHashMap<Symbol, std::size_t>::type Offsets;

  JSDeclEnv(JSEnv* outer, uint32_t scope_nest_count)
    : JSEnv(outer),
      record_(),
      offsets_(),
      scope_nest_count_(scope_nest_count),
      mutated_(false) {
  }

  // for VM optimization only
  JSDeclEnv(JSEnv* outer, uint32_t scope_nest_count, std::size_t reserved)
    : JSEnv(outer),
      record_(reserved),
      offsets_(),
      scope_nest_count_(scope_nest_count),
      mutated_(false) {
    assert(record_.size() == reserved);
  }

  bool HasBinding(Context* ctx, Symbol name) const {
    return offsets_.find(name) != offsets_.end();
  }

  bool DeleteBinding(Context* ctx, Symbol name) {
    const Offsets::const_iterator it = offsets_.find(name);
    if (it == offsets_.end()) {
      return true;
    }
    if (record_[it->second].attribute & DELETABLE) {
      record_[it->second] = Entry();
      offsets_.erase(it);
      return true;
    } else {
      return false;
    }
  }

  void CreateMutableBinding(Context* ctx, Symbol name, bool del, Error* e) {
    assert(offsets_.find(name) == offsets_.end());
    int flag = MUTABLE;
    if (del) {
      flag |= DELETABLE;
    }
    offsets_[name] = record_.size();
    record_.push_back(Entry(flag));
  }

  void SetMutableBinding(Context* ctx,
                         Symbol name,
                         const JSVal& val,
                         bool strict, Error* e) {
    const Offsets::const_iterator it = offsets_.find(name);
    assert(it != offsets_.end());
    if (record_[it->second].attribute & MUTABLE) {
      record_[it->second].value = val;
    } else {
      if (strict) {
        e->Report(Error::Type, "mutating immutable binding not allowed");
      }
    }
  }

  JSVal GetBindingValue(Context* ctx,
                        Symbol name, bool strict, Error* e) const {
    const Offsets::const_iterator it = offsets_.find(name);
    assert(it != offsets_.end());
    if (record_[it->second].attribute & IM_UNINITIALIZED) {
      if (strict) {
        e->Report(Error::Reference,
                  "uninitialized value access not allowed in strict code");
      }
      return JSUndefined;
    } else {
      return record_[it->second].value;
    }
  }

  JSVal GetBindingValue(Symbol name) const {
    const Offsets::const_iterator it = offsets_.find(name);
    assert(it != offsets_.end() && !(record_[it->second].attribute & IM_UNINITIALIZED));
    return record_[it->second].value;
  }

  JSVal ImplicitThisValue() const {
    return JSUndefined;
  }

  void CreateImmutableBinding(Symbol name) {
    assert(offsets_.find(name) == offsets_.end());
    offsets_[name] = record_.size();
    record_.push_back(Entry(IM_UNINITIALIZED));
  }

  void InitializeImmutableBinding(Symbol name, const JSVal& val) {
    assert(offsets_.find(name) != offsets_.end() &&
           (record_[offsets_.find(name)->second].attribute & IM_UNINITIALIZED));
    record_[offsets_.find(name)->second] = Entry(IM_INITIALIZED, val);
  }

  JSDeclEnv* AsJSDeclEnv() {
    return this;
  }

  JSObjectEnv* AsJSObjectEnv() {
    return NULL;
  }

  JSStaticEnv* AsJSStaticEnv() {
    return NULL;
  }

  Record& record() {
    return record_;
  }

  const Record& record() const {
    return record_;
  }

  static JSDeclEnv* New(Context* ctx,
                        JSEnv* outer,
                        uint32_t scope_nest_count) {
    return new JSDeclEnv(outer, scope_nest_count);
  }

  static JSDeclEnv* New(Context* ctx,
                        JSEnv* outer,
                        uint32_t scope_nest_count,
                        uint32_t reserved) {
    return new JSDeclEnv(outer, scope_nest_count, reserved);
  }

  bool IsLookupNeeded() const {
    return mutated_;
  }

  uint32_t scope_nest_count() const {
    return scope_nest_count_;
  }

  void MarkMutated() {
    mutated_ = true;
  }

  // for VM optimization methods
  void CreateAndSetMutable(Symbol name, std::size_t offset, const JSVal& val) {
    offsets_[name] = offset;
    record_[offset] = Entry(MUTABLE, val);
  }

  void CreateAndSetImmutable(Symbol name, std::size_t offset, const JSVal& val) {
    offsets_[name] = offset;
    record_[offset] = Entry(IM_INITIALIZED, val);
  }

  void CreateFDecl(Symbol name, std::size_t offset) {
    offsets_[name] = offset;
    record_[offset] = Entry(MUTABLE);
  }

  void SetByOffset(uint32_t offset, const JSVal& value, bool strict, Error* e) {
    Entry& entry = record_[offset];
    if (entry.attribute & MUTABLE) {
      entry.value = value;
    } else {
      if (strict) {
        e->Report(Error::Type, "mutating immutable binding not allowed");
      }
    }
  }

  JSVal GetByOffset(uint32_t offset, bool strict, Error* e) const {
    const Entry& entry = record_[offset];
    if (entry.attribute & IM_UNINITIALIZED) {
      if (strict) {
        e->Report(Error::Reference,
                  "uninitialized value access not allowed in strict code");
      }
      return JSUndefined;
    }
    return entry.value;
  }

 private:
  Record record_;
  Offsets offsets_;
  uint32_t scope_nest_count_;
  bool mutated_;
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
        builder.Append(symbol::GetSymbolString(name));
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

  JSStaticEnv* AsJSStaticEnv() {
    return NULL;
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

  bool IsLookupNeeded() const {
    return true;
  }

 private:
  JSObject* record_;
  bool provide_this_;
};

// for catch block environment
class JSStaticEnv : public JSEnv {
 public:
  explicit JSStaticEnv(JSEnv* outer, Symbol sym, const JSVal& value)
    : JSEnv(outer),
      symbol_(sym),
      value_(value) {
  }

  bool HasBinding(Context* ctx, Symbol name) const {
    return name == symbol_;
  }

  bool DeleteBinding(Context* ctx, Symbol name) {
    return name != symbol_;
  }

  void CreateMutableBinding(Context* ctx, Symbol name, bool del, Error* err) {
    UNREACHABLE();
  }

  void SetMutableBinding(Context* ctx,
                         Symbol name,
                         const JSVal& val,
                         bool strict, Error* res) {
    assert(name == symbol_);
    value_ = val;
  }

  JSVal GetBindingValue(Context* ctx, Symbol name,
                        bool strict, Error* res) const {
    assert(name == symbol_);
    return value_;
  }

  JSVal ImplicitThisValue() const {
    return JSUndefined;
  }

  JSDeclEnv* AsJSDeclEnv() {
    return NULL;
  }

  JSObjectEnv* AsJSObjectEnv() {
    return NULL;
  }

  JSStaticEnv* AsJSStaticEnv() {
    return this;
  }

  static JSStaticEnv* New(Context* ctx, JSEnv* outer,
                          Symbol sym, const JSVal& value) {
    return new JSStaticEnv(outer, sym, value);
  }

  bool IsLookupNeeded() const {
    return true;
  }

 private:
  Symbol symbol_;
  JSVal value_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSENV_H_
