#include <cstdio>
#include <utility>
#include <tr1/array>
#include "stringpiece.h"
#include "jsobject.h"
#include "jsenv.h"
#include "context.h"
#include "jsval.h"
#include "jsarguments.h"
#include "error.h"
namespace iv {
namespace lv5 {

JSArguments* JSArguments::New(Context* ctx,
                              JSCodeFunction* code,
                              const Identifiers& names,
                              const Arguments& args,
                              JSDeclEnv* env,
                              bool strict) {
  JSArguments* const obj = new JSArguments(ctx, env);
  const std::size_t len = args.size();
  const std::size_t names_len = names.size();
  const Class& cls = ctx->Cls(ctx->Intern("Arguments"));
  obj->set_cls(cls.name);
  obj->set_prototype(cls.prototype);
  obj->DefineOwnProperty(ctx, ctx->length_symbol(),
                         DataDescriptor(static_cast<double>(len),
                                        PropertyDescriptor::WRITABLE |
                                        PropertyDescriptor::CONFIGURABLE),
                         false, NULL);
  std::size_t index = len - 1;
  std::tr1::array<char, std::numeric_limits<std::size_t>::digits10 + 1> buf;
  for (Arguments::const_reverse_iterator it = args.rbegin(),
       last = args.rend(); it != last; ++it) {
    int len = std::snprintf(buf.data(), buf.size(),
                            "%lu",
                            static_cast<unsigned long>(index));  // NOLINT
    const Symbol index_symbol = ctx->Intern(core::StringPiece(buf.data(), len));
    obj->DefineOwnProperty(ctx, index_symbol,
                           DataDescriptor(*it,
                                          PropertyDescriptor::WRITABLE |
                                          PropertyDescriptor::ENUMERABLE |
                                          PropertyDescriptor::CONFIGURABLE),
                           false, NULL);
    if (index < names_len) {
      if (!strict) {
        obj->map_.insert(
            std::make_pair(index_symbol, ctx->Intern((*names[index]))));
      }
    }
    index -= 1;
  }
  if (strict) {
    JSNativeFunction* const throw_type_error = ctx->throw_type_error();
    obj->DefineOwnProperty(ctx, ctx->caller_symbol(),
                           AccessorDescriptor(throw_type_error,
                                              throw_type_error,
                                              PropertyDescriptor::NONE),
                           false, NULL);
    obj->DefineOwnProperty(ctx, ctx->callee_symbol(),
                           AccessorDescriptor(throw_type_error,
                                              throw_type_error,
                                              PropertyDescriptor::NONE),
                           false, NULL);
  } else {
    obj->DefineOwnProperty(ctx, ctx->callee_symbol(),
                           DataDescriptor(code,
                                          PropertyDescriptor::WRITABLE |
                                          PropertyDescriptor::CONFIGURABLE),
                           false, NULL);
  }
  return obj;
}

JSVal JSArguments::Get(Context* ctx,
                       Symbol name, Error* error) {
  const Index2Param::const_iterator it = map_.find(name);
  if (it != map_.end()) {
    return env_->GetBindingValue(ctx, it->second, true, error);
  } else {
    const JSVal v = JSObject::Get(ctx, name, error);
    if (*error) {
      return JSUndefined;
    }
    if (name == ctx->caller_symbol() &&
        v.IsCallable() &&
        v.object()->AsCallable()->IsStrict()) {
      error->Report(Error::Type,
                    "access to strict function \"caller\" not allowed");
      return JSUndefined;
    }
    return v;
  }
}

PropertyDescriptor JSArguments::GetOwnProperty(Symbol name) const {
  const PropertyDescriptor desc = JSObject::GetOwnProperty(name);
  if (desc.IsEmpty()) {
    return desc;
  }
  if (map_.find(name) != map_.end()) {
    const JSVal val = env_->GetBindingValue(name);
    return DataDescriptor(val);
  }
  return desc;
}

bool JSArguments::DefineOwnProperty(Context* ctx,
                                    Symbol name,
                                    const PropertyDescriptor& desc,
                                    bool th,
                                    Error* error) {
  const bool allowed = JSObject::DefineOwnProperty(ctx, name,
                                                   desc, false, NULL);
  if (!allowed) {
    if (th) {
      error->Report(Error::Type,
                    "[[DefineOwnProperty]] failed");
    }
    return false;
  }
  const Index2Param::const_iterator it = map_.find(name);
  if (it != map_.end()) {
    if (desc.IsAccessorDescriptor()) {
      map_.erase(it);
    } else {
      // TODO(Constellation) [[Value]] check
      env_->SetMutableBinding(ctx,
                              name, desc.AsDataDescriptor()->data(),
                              th, error);
      if (*error) {
        return false;
      }
      if (!desc.IsWritableAbsent() && !desc.IsWritable()) {
        map_.erase(it);
      }
    }
  }
  return true;
}

bool JSArguments::Delete(Symbol name, bool th, Error* error) {
  const bool result = JSObject::Delete(name, th, error);
  if (*error) {
    return result;
  }
  if (result) {
    const Index2Param::const_iterator it = map_.find(name);
    if (it != map_.end()) {
      map_.erase(it);
      return true;
    }
  }
  return result;
}

} }  // namespace iv::lv5
