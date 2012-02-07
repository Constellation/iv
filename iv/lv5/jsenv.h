#ifndef IV_LV5_JSENV_H_
#define IV_LV5_JSENV_H_
#include <gc/gc_cpp.h>
#include <iv/notfound.h>
#include <iv/debug.h>
#include <iv/string_builder.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/property_fwd.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/core_fwd.h>

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
  inline JSEnv* outer() const {
    return outer_;
  }

  void MarkChildren(radio::Core* core) {
    core->MarkCell(outer_);
  }

 protected:
  explicit JSEnv(JSEnv* outer)
    : outer_(outer) {
  }

 private:
  JSEnv* outer_;
};

class JSDeclEnv : public JSEnv {
 private:
  class Entry {
   public:
    enum RecordType {
      NONE = 0,
      IM_INITIALIZED = 1,
      IM_UNINITIALIZED = 2,
      MUTABLE = 4,
      DELETABLE = 8,
      REDIRECT = 16
    };

    Entry()
      : attribute_(NONE),
        value_or_redirect_(JSEmpty) {
    }

    explicit Entry(int attr)
      : attribute_(attr),
        value_or_redirect_(JSUndefined) {
    }

    Entry(int attr, const JSVal& val)
      : attribute_(attr),
        value_or_redirect_(val) {
    }

    Entry(int attr, JSVal* val)
      : attribute_(attr | REDIRECT),
        value_or_redirect_(val) {
    }

    JSVal value() const {
      return (attribute_ & REDIRECT) ?
          *value_or_redirect_.pointer() : value_or_redirect_.value();
    }

    void set_value(JSVal value) {
      value_or_redirect_.set_value(value);
    }

    int attribute() const { return attribute_; }

   private:
    int attribute_;
    JSValOrRedirect value_or_redirect_;
  };

 public:
  typedef GCVector<Entry>::type Record;
  typedef GCHashMap<Symbol, std::size_t>::type Offsets;

  bool HasBinding(Context* ctx, Symbol name) const {
    return offsets_.find(name) != offsets_.end();
  }

  bool DeleteBinding(Context* ctx, Symbol name) {
    const Offsets::const_iterator it = offsets_.find(name);
    if (it == offsets_.end()) {
      return true;
    }
    if (record_[it->second].attribute() & Entry::DELETABLE) {
      record_[it->second] = Entry();
      offsets_.erase(it);
      return true;
    } else {
      return false;
    }
  }

  void CreateMutableBinding(Context* ctx, Symbol name, bool del, Error* e) {
    assert(offsets_.find(name) == offsets_.end());
    int flag = Entry::MUTABLE;
    if (del) {
      flag |= Entry::DELETABLE;
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
    if (record_[it->second].attribute() & Entry::MUTABLE) {
      record_[it->second].set_value(val);
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
    if (record_[it->second].attribute() & Entry::IM_UNINITIALIZED) {
      if (strict) {
        e->Report(Error::Reference,
                  "uninitialized value access not allowed in strict code");
      }
      return JSUndefined;
    } else {
      return record_[it->second].value();
    }
  }

  JSVal GetBindingValue(Symbol name) const {
    const Offsets::const_iterator it = offsets_.find(name);
    assert(it != offsets_.end());
    assert(!(record_[it->second].attribute() & Entry::IM_UNINITIALIZED));
    return record_[it->second].value();
  }

  JSVal ImplicitThisValue() const {
    return JSUndefined;
  }

  void CreateImmutableBinding(Symbol name) {
    assert(offsets_.find(name) == offsets_.end());
    offsets_[name] = record_.size();
    record_.push_back(Entry(Entry::IM_UNINITIALIZED));
  }

  void InitializeImmutableBinding(Symbol name, const JSVal& val) {
    assert(offsets_.find(name) != offsets_.end());
    assert(record_[offsets_.find(name)->second].attribute() &
           Entry::IM_UNINITIALIZED);
    record_[offsets_.find(name)->second] = Entry(Entry::IM_INITIALIZED, val);
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

  static JSDeclEnv* New(Context* ctx, JSEnv* outer) {
    return new JSDeclEnv(outer);
  }

  static JSDeclEnv* New(Context* ctx, JSEnv* outer, uint32_t reserved) {
    return new JSDeclEnv(outer, reserved);
  }

  // for VM optimization methods
  void InstantiateMutable(Symbol name, std::size_t offset) {
    offsets_[name] = offset;
    record_[offset] = Entry(Entry::MUTABLE, JSUndefined);
  }

  void InstantiateImmutable(Symbol name, std::size_t offset) {
    offsets_[name] = offset;
  }

  void InitializeImmutable(std::size_t offset, const JSVal& val) {
    record_[offset] = Entry(Entry::IM_INITIALIZED, val);
  }

  void SetByOffset(uint32_t offset, const JSVal& value, bool strict, Error* e) {
    Entry& entry = record_[offset];
    if (entry.attribute() & Entry::MUTABLE) {
      entry.set_value(value);
    } else {
      if (strict) {
        e->Report(Error::Type, "mutating immutable binding not allowed");
      }
    }
  }

  JSVal GetByOffset(uint32_t offset, bool strict, Error* e) const {
    const Entry& entry = record_[offset];
    if (entry.attribute() & Entry::IM_UNINITIALIZED) {
      if (strict) {
        e->Report(Error::Reference,
                  "uninitialized value access not allowed in strict code");
      }
      return JSUndefined;
    }
    return entry.value();
  }

  void MarkChildren(radio::Core* core) {
    JSEnv::MarkChildren(core);
    for (Record::const_iterator it = record_.begin(),
         last = record_.end(); it != last; ++it) {
      core->MarkValue(it->value());
    }
  }

 private:
  JSDeclEnv(JSEnv* outer)
    : JSEnv(outer),
      record_(),
      offsets_() {
  }

  // for VM optimization only
  JSDeclEnv(JSEnv* outer, std::size_t reserved)
    : JSEnv(outer),
      record_(reserved),
      offsets_() {
    assert(record_.size() == reserved);
  }

  Record record_;
  Offsets offsets_;
};

class JSObjectEnv : public JSEnv {
 public:
  bool HasBinding(Context* ctx, Symbol name) const {
    return record_->HasProperty(ctx, name);
  }

  bool DeleteBinding(Context* ctx, Symbol name) {
    return record_->Delete(ctx, name, false, NULL);
  }

  void CreateMutableBinding(Context* ctx, Symbol name, bool del, Error* err) {
    assert(!record_->HasProperty(ctx, name));
    int attr = ATTR::WRITABLE | ATTR::ENUMERABLE;
    if (del) {
      attr |= ATTR::CONFIGURABLE;
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
        core::UStringBuilder builder;
        builder.Append('"');
        builder.Append(symbol::GetSymbolString(name));
        builder.Append("\" not defined");
        res->Report(Error::Reference, builder.BuildPiece());
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

  void MarkChildren(radio::Core* core) {
    JSEnv::MarkChildren(core);
    core->MarkCell(record_);
  }

 private:
  explicit JSObjectEnv(JSEnv* outer, JSObject* rec)
    : JSEnv(outer),
      record_(rec),
      provide_this_(false) {
  }

  JSObject* record_;
  bool provide_this_;
};

// for catch block environment
class JSStaticEnv : public JSEnv {
 public:
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

  void MarkChildren(radio::Core* core) {
    JSEnv::MarkChildren(core);
    core->MarkValue(value_);
  }

 private:
  explicit JSStaticEnv(JSEnv* outer, Symbol sym, const JSVal& value)
    : JSEnv(outer),
      symbol_(sym),
      value_(value) {
  }

  Symbol symbol_;
  JSVal value_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSENV_H_
