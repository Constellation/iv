#ifndef IV_LV5_JSENV_H_
#define IV_LV5_JSENV_H_
#include <algorithm>
#include <gc/gc_cpp.h>
#include <iv/notfound.h>
#include <iv/debug.h>
#include <iv/string_builder.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/property_fwd.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/storage.h>
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
                                 JSVal val,
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

  static std::size_t OuterOffset() {
    return IV_OFFSETOF(JSEnv, outer_);
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
  class DynamicVal {
   public:
    enum RecordType {
      NONE = 0,
      IM_INITIALIZED = 1,
      IM_UNINITIALIZED = 2,
      MUTABLE = 4,
      DELETABLE = 8
    };

    DynamicVal()
      : attribute_(NONE),
        escaped_(JSEmpty) {
    }

    DynamicVal(const DynamicVal& rhs)
      : attribute_(rhs.attribute_),
        escaped_(rhs.escaped_) {
    }

    explicit DynamicVal(int attr)
      : attribute_(attr),
        escaped_(JSUndefined) {
    }

    DynamicVal(int attr, JSVal* reg)
      : attribute_(attr),
        escaped_(JSEmpty) {
    }

    DynamicVal(int attr, JSVal val)
      : attribute_(attr),
        escaped_(val) {
    }

    JSVal value() const { return escaped_; }

    void set_value(JSVal value) { escaped_ = value; }

    int attribute() const { return attribute_; }

    void set_attribute(int attr) { attribute_ = attr; }

    DynamicVal& operator=(const DynamicVal& rhs) {
      using std::swap;
      attribute_ = rhs.attribute_;
      escaped_ = rhs.escaped_;
      return *this;
    }

    void swap(DynamicVal& rhs) {
      using std::swap;
      swap(attribute_, rhs.attribute_);
      swap(escaped_, rhs.escaped_);
    }

    friend void swap(DynamicVal& lhs, DynamicVal& rhs) {
      lhs.swap(rhs);
    }

   private:
    int attribute_;
    JSVal escaped_;
  };

 public:
  typedef Storage<JSVal> StaticVals;
  typedef Storage<DynamicVal> DynamicVals;
  typedef GCHashMap<Symbol, uint32_t>::type Offsets;

  bool HasBinding(Context* ctx, Symbol name) const {
    return offsets_.find(name) != offsets_.end();
  }

  bool DeleteBinding(Context* ctx, Symbol name) {
    const Offsets::const_iterator it = offsets_.find(name);
    if (it == offsets_.end()) {
      return true;
    }
    const uint32_t offset = it->second;
    if (offset >= static_.size()) {
      const DynamicVal& dyn = dynamic_[offset - static_.size()];
      if (dyn.attribute() & DynamicVal::DELETABLE) {
        dynamic_[offset - static_.size()] = DynamicVal();
        offsets_.erase(it);
        return true;
      }
    }
    return false;
  }

  void CreateMutableBinding(Context* ctx, Symbol name, bool del, Error* e) {
    assert(offsets_.find(name) == offsets_.end());
    int flag = DynamicVal::MUTABLE;
    if (del) {
      flag |= DynamicVal::DELETABLE;
    }
    offsets_[name] = static_.size() + dynamic_.size();
    dynamic_.push_back(DynamicVal(flag));
  }

  virtual void SetMutableBinding(Context* ctx,
                                 Symbol name,
                                 JSVal val,
                                 bool strict, Error* e) {
    assert(offsets_.find(name) != offsets_.end());
    SetByOffset(offsets_.find(name)->second, val, strict, e);
  }

  virtual JSVal GetBindingValue(Context* ctx,
                                Symbol name, bool strict, Error* e) const {
    assert(offsets_.find(name) != offsets_.end());
    return GetByOffset(offsets_.find(name)->second, strict, e);
  }

  virtual JSVal ImplicitThisValue() const {
    return JSUndefined;
  }

  void CreateImmutableBinding(Symbol name) {
    assert(offsets_.find(name) == offsets_.end());
    offsets_[name] = static_.size() + dynamic_.size();
    dynamic_.push_back(DynamicVal(DynamicVal::IM_UNINITIALIZED));
  }

  void InitializeImmutableBinding(Symbol name, JSVal val) {
    assert(offsets_.find(name) != offsets_.end());
    InitializeImmutableBinding(offsets_.find(name)->second, val);
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

  // for railgun::VM optimization methods

  template<typename NamesIter>
  static JSDeclEnv* New(Context* ctx,
                        JSEnv* outer,
                        uint32_t size,
                        NamesIter it,
                        uint32_t mutable_start) {
    return new JSDeclEnv(outer, size, it, mutable_start);
  }

  void InitializeImmutableBinding(uint32_t offset, JSVal val) {
    if (offset < mutable_start()) {
      assert(static_[offset].IsEmpty());
      static_[offset] = val;
      return;
    }
    assert(offset >= static_.size());
    offset -= static_.size();
    assert(dynamic_[offset].attribute() & DynamicVal::IM_UNINITIALIZED);
    dynamic_[offset] = DynamicVal(DynamicVal::IM_INITIALIZED, val);
  }

  void SetByOffset(uint32_t offset, JSVal val, bool strict, Error* e) {
    if (offset < static_.size()) {
      if (offset < mutable_start()) {
        if (strict) {
          e->Report(Error::Type, "mutating immutable binding not allowed");
        }
      } else {
        static_[offset] = val;
      }
      return;
    }
    DynamicVal& dyn = dynamic_[offset - static_.size()];
    if (dyn.attribute() & DynamicVal::MUTABLE) {
      dyn.set_value(val);
    } else {
      if (strict) {
        e->Report(Error::Type, "mutating immutable binding not allowed");
      }
    }
  }

  JSVal GetByOffset(uint32_t offset, bool strict, Error* e) const {
    if (offset < static_.size()) {
      const JSVal ret = static_[offset];
      if (offset < mutable_start()) {
        if (ret.IsEmpty()) {
          if (strict) {
            e->Report(Error::Reference,
                      "uninitialized value access not allowed in strict code");
          }
          return JSUndefined;
        }
      }
      return ret;
    }
    const DynamicVal& dyn = dynamic_[offset - static_.size()];
    if (dyn.attribute() & DynamicVal::IM_UNINITIALIZED) {
      if (strict) {
        e->Report(Error::Reference,
                  "uninitialized value access not allowed in strict code");
      }
      return JSUndefined;
    }
    return dyn.value();
  }

  void MarkChildren(radio::Core* core) {
    JSEnv::MarkChildren(core);
    std::for_each(static_.begin(), static_.end(), radio::Core::Marker(core));
    for (DynamicVals::const_iterator it = dynamic_.begin(),
         last = dynamic_.end(); it != last; ++it) {
      core->MarkValue(it->value());
    }
  }

  static std::size_t StaticOffset() { return IV_OFFSETOF(JSDeclEnv, static_); }

 private:
  JSDeclEnv(JSEnv* outer)
    : JSEnv(outer),
      mutable_start_(0),
      static_(),
      dynamic_(),
      offsets_() {
  }

  // for VM optimization only
  template<typename NamesIter>
  JSDeclEnv(JSEnv* outer,
            uint32_t size,
            NamesIter it, uint32_t mutable_start)
    : JSEnv(outer),
      mutable_start_(mutable_start),
      static_(size, JSUndefined),
      dynamic_(),
      offsets_() {
    uint32_t i = 0;
    for (; i < mutable_start; ++i, ++it) {
      static_[i] = JSEmpty;
      offsets_[*it] = i;
    }
    for (; i < size; ++i, ++it) {
      offsets_[*it] = i;
    }
  }

  uint32_t mutable_start() const { return mutable_start_; }

  uint32_t mutable_start_;
  StaticVals static_;
  DynamicVals dynamic_;
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
                         JSVal val,
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
                         JSVal val,
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
                          Symbol sym, JSVal value) {
    return new JSStaticEnv(outer, sym, value);
  }

  void MarkChildren(radio::Core* core) {
    JSEnv::MarkChildren(core);
    core->MarkValue(value_);
  }

 private:
  explicit JSStaticEnv(JSEnv* outer, Symbol sym, JSVal value)
    : JSEnv(outer),
      symbol_(sym),
      value_(value) {
  }

  Symbol symbol_;
  JSVal value_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSENV_H_
