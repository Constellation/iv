#include <cassert>
#include "jsenv.h"
#include "property.h"
#include "jsval.h"
#include "context.h"

namespace iv {
namespace lv5 {

bool JSDeclEnv::HasBinding(Symbol name) const {
  return record_.find(name) != record_.end();
}

bool JSDeclEnv::DeleteBinding(Symbol name) {
  Record::const_iterator it(record_.find(name));
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

void JSDeclEnv::CreateMutableBinding(Context* ctx, Symbol name, bool del) {
  assert(record_.find(name) == record_.end());
  int flag = MUTABLE;
  if (del) {
    flag |= DELETABLE;
  }
  record_[name] = std::make_pair(flag, JSUndefined);
}

void JSDeclEnv::SetMutableBinding(Context* ctx,
                                  Symbol name,
                                  const JSVal& val,
                                  bool strict, JSErrorCode::Type* res) {
  Record::const_iterator it(record_.find(name));
  assert(it != record_.end());
  if (it->second.first & MUTABLE) {
    record_[name] = std::make_pair(it->second.first, val);
  } else {
    if (strict) {
      *res = JSErrorCode::TypeError;
    }
  }
}

JSVal JSDeclEnv::GetBindingValue(Context* ctx,
                                 Symbol name,
                                 bool strict, JSErrorCode::Type* res) const {
  Record::const_iterator it(record_.find(name));
  assert(it != record_.end());
  if (it->second.first & IM_UNINITIALIZED) {
    if (strict) {
      *res = JSErrorCode::ReferenceError;
    }
    return JSUndefined;
  } else {
    return it->second.second;
  }
}

JSVal JSDeclEnv::ImplicitThisValue() const {
  return JSUndefined;
}

void JSDeclEnv::CreateImmutableBinding(Symbol name) {
  assert(record_.find(name) == record_.end());
  record_[name] = std::make_pair(IM_UNINITIALIZED, JSUndefined);
}

void JSDeclEnv::InitializeImmutableBinding(Symbol name, const JSVal& val) {
  assert(record_.find(name) != record_.end() &&
         (record_.find(name)->second.first & IM_UNINITIALIZED));
  record_[name] = std::make_pair(IM_INITIALIZED, val);
}

JSDeclEnv* JSDeclEnv::New(Context* ctx, JSEnv* outer) {
  return new JSDeclEnv(outer);
}


bool JSObjectEnv::HasBinding(Symbol name) const {
  return record_->HasProperty(name);
}

bool JSObjectEnv::DeleteBinding(Symbol name) {
  return record_->Delete(name, false, NULL);
}

void JSObjectEnv::CreateMutableBinding(Context* ctx, Symbol name, bool del) {
  assert(!record_->HasProperty(name));
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
      NULL);
}

void JSObjectEnv::SetMutableBinding(Context* ctx,
                                    Symbol name,
                                    const JSVal& val,
                                    bool strict, JSErrorCode::Type* res) {
  record_->Put(ctx, name, val, strict, res);
}

JSVal JSObjectEnv::GetBindingValue(Context* ctx,
                                   Symbol name,
                                   bool strict, JSErrorCode::Type* res) const {
  bool value = record_->HasProperty(name);
  if (!value) {
    if (strict) {
      *res = JSErrorCode::ReferenceError;
    }
    return JSUndefined;
  }
  return record_->Get(ctx, name, res);
}

JSVal JSObjectEnv::ImplicitThisValue() const {
  if (provide_this_) {
    return record_;
  } else {
    return JSUndefined;
  }
}

JSObjectEnv* JSObjectEnv::New(Context* ctx, JSEnv* outer, JSObject* rec) {
  return new JSObjectEnv(outer, rec);
}

} }  // namespace iv::lv5
