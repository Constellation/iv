#include <cassert>
#include "jsobject.h"
#include "property.h"
#include "jsfunction.h"
#include "jsval.h"
#include "jsenv.h"
#include "context.h"
#include "class.h"

namespace iv {
namespace lv5 {

JSObject::JSObject()
  : prototype_(NULL),
    cls_(NULL),
    extensible_(true),
    table_() {
}

JSObject::JSObject(JSObject* proto,
                   JSString* cls,
                   bool extensible)
  : prototype_(proto),
    cls_(cls),
    extensible_(extensible),
    table_() {
}

#define TRY(context, sym, arg, error)\
  do {\
    JSVal method = Get(context, sym, error);\
    if (*error) {\
      return JSUndefined;\
    }\
    if (method.IsCallable()) {\
      JSVal val = method.object()->AsCallable()->Call(arg, error);\
      if (*error) {\
        return JSUndefined;\
      }\
      if (val.IsPrimitive()) {\
        return val;\
      }\
    }\
  } while (0)
JSVal JSObject::DefaultValue(Context* ctx,
                             Hint::Object hint, Error* res) {
  const Arguments args(ctx, this);
  if (hint != Hint::NUMBER) {
    // hint is STRING or NONE
    TRY(ctx, ctx->toString_symbol(), args, res);
    TRY(ctx, ctx->valueOf_symbol(), args, res);
  } else {
    TRY(ctx, ctx->valueOf_symbol(), args, res);
    TRY(ctx, ctx->toString_symbol(), args, res);
  }
  res->Report(Error::Type, "invalid default value");
  return JSUndefined;
}
#undef TRY

JSVal JSObject::Get(Context* ctx,
                    Symbol name, Error* res) {
  const PropertyDescriptor desc = GetProperty(name);
  if (desc.IsEmpty()) {
    return JSUndefined;
  }
  if (desc.IsDataDescriptor()) {
    return desc.AsDataDescriptor()->data();
  } else {
    assert(desc.IsAccessorDescriptor());
    JSObject* const getter = desc.AsAccessorDescriptor()->get();
    if (getter) {
      return getter->AsCallable()->Call(Arguments(ctx, this), res);
    } else {
      return JSUndefined;
    }
  }
}

// not recursion
PropertyDescriptor JSObject::GetProperty(Symbol name) const {
  const JSObject* obj = this;
  do {
    const PropertyDescriptor prop = obj->GetOwnProperty(name);
    if (!prop.IsEmpty()) {
      return prop;
    }
    obj = obj->prototype();
  } while (obj);
  return JSUndefined;
}

PropertyDescriptor JSObject::GetOwnProperty(Symbol name) const {
  const Properties::const_iterator it = table_.find(name);
  if (it == table_.end()) {
    return JSUndefined;
  } else {
    return it->second;
  }
}

bool JSObject::CanPut(Symbol name) const {
  const PropertyDescriptor desc = GetOwnProperty(name);
  if (!desc.IsEmpty()) {
    if (desc.IsAccessorDescriptor()) {
      return desc.AsAccessorDescriptor()->set();
    } else {
      assert(desc.IsDataDescriptor());
      return desc.IsWritable();
    }
  }
  if (!prototype_) {
    return extensible_;
  }
  const PropertyDescriptor inherited = prototype_->GetProperty(name);
  if (inherited.IsEmpty()) {
    return extensible_;
  } else {
    if (inherited.IsAccessorDescriptor()) {
      return inherited.AsAccessorDescriptor()->set();
    } else {
      assert(inherited.IsDataDescriptor());
      return inherited.IsWritable();
    }
  }
}

#define REJECT(str)\
  do {\
    if (th) {\
      res->Report(Error::Type, str);\
    }\
    return false;\
  } while (0)

bool JSObject::DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th,
                                 Error* res) {
  // section 8.12.9 [[DefineOwnProperty]]
  const PropertyDescriptor current = GetOwnProperty(name);
  if (current.IsEmpty()) {
    if (!extensible_) {
      REJECT("object not extensible");
    } else {
      assert(desc.IsDataDescriptor() || desc.IsAccessorDescriptor());
      table_[name] = desc.SetDefaultToAbsent();
      return true;
    }
  }

  // step 5 : this interpreter not allows absent descriptor
  // step 6
  if (desc.Equals(current)) {
    return true;
  }

  // step 7
  if (!current.IsConfigurable()) {
    if (desc.Configurable() == PropertyDescriptor::kTRUE) {
      REJECT(
          "changing [[Configurable]] of unconfigurable property not allowed");
    }
    if (!desc.IsEnumerableAbsent() &&
        current.Enumerable() != desc.Enumerable()) {
      REJECT("changing [[Enumerable]] of unconfigurable property not allowed");
    }
  }

  // step 9
  if (current.type() != desc.type()) {
    if (!current.Configurable()) {
      REJECT("changing descriptor type of unconfigurable property not allowed");
    }
    if (current.IsDataDescriptor()) {
      assert(desc.IsAccessorDescriptor());
    } else {
      assert(desc.IsDataDescriptor());
    }
  } else {
    // step 10
    if (current.IsDataDescriptor()) {
      assert(desc.IsDataDescriptor());
      if (!current.IsConfigurable()) {
        if (!current.IsWritable()) {
          if (desc.Writable() == PropertyDescriptor::kTRUE) {
            REJECT(
                "changing [[Writable]] of unconfigurable property not allowed");
          }
          if (SameValue(current.AsDataDescriptor()->data(),
                        desc.AsDataDescriptor()->data())) {
            REJECT("changing [[Value]] of readonly property not allowed");
          }
        }
      }
    } else {
      // step 11
      assert(desc.IsAccessorDescriptor());
      if (!current.Configurable()) {
        const AccessorDescriptor* const lhs = current.AsAccessorDescriptor();
        const AccessorDescriptor* const rhs = desc.AsAccessorDescriptor();
        if (lhs->set() != rhs->set() || lhs->get() != rhs->get()) {
          REJECT("changing [[Set]] or [[Get]] "
                 "of unconfigurable property not allowed");
        }
      }
    }
  }
  table_[name] = desc.MergeAttrs(current.attrs());
  return true;
}

#undef REJECT

void JSObject::Put(Context* ctx,
                   Symbol name,
                   const JSVal& val, bool th, Error* res) {
  if (!CanPut(name)) {
    if (th) {
      res->Report(Error::Type, "put failed");
    }
    return;
  }
  const PropertyDescriptor own_desc = GetOwnProperty(name);
  if (!own_desc.IsEmpty() && own_desc.IsDataDescriptor()) {
    DefineOwnProperty(ctx, name, DataDescriptor(val), th, res);
    return;
  }
  const PropertyDescriptor desc = GetProperty(name);
  if (!desc.IsEmpty() && desc.IsAccessorDescriptor()) {
    const AccessorDescriptor* const accs = desc.AsAccessorDescriptor();
    assert(accs->set());
    accs->set()->AsCallable()->Call(Arguments(ctx, this), res);
  } else {
    DefineOwnProperty(ctx, name,
                      DataDescriptor(val,
                                     PropertyDescriptor::WRITABLE |
                                     PropertyDescriptor::ENUMERABLE |
                                     PropertyDescriptor::CONFIGURABLE),
                      th, res);
  }
}

bool JSObject::HasProperty(Symbol name) const {
  return !GetProperty(name).IsEmpty();
}

bool JSObject::Delete(Symbol name, bool th, Error* res) {
  const PropertyDescriptor desc = GetOwnProperty(name);
  if (desc.IsEmpty()) {
    return true;
  }
  if (desc.IsConfigurable()) {
    table_.erase(name);
    return true;
  } else {
    if (th) {
      res->Report(Error::Type, "delete failed");
    }
    return false;
  }
}

JSObject* JSObject::New(Context* ctx) {
  JSObject* const obj = NewPlain(ctx);
  const Symbol name = ctx->Intern("Object");
  const Class& cls = ctx->Cls(name);
  obj->set_cls(cls.name);
  obj->set_prototype(cls.prototype);
  return obj;
}

JSObject* JSObject::NewPlain(Context* ctx) {
  return new JSObject();
}

JSStringObject* JSStringObject::New(Context* ctx, JSString* str) {
  JSStringObject* const obj = new JSStringObject(str);
  const Symbol name = ctx->Intern("String");
  const Class& cls = ctx->Cls(name);
  obj->set_cls(cls.name);
  obj->set_prototype(cls.prototype);
  return obj;
}

JSStringObject* JSStringObject::NewPlain(Context* ctx) {
  return new JSStringObject(JSString::NewEmptyString(ctx));
}

JSNumberObject* JSNumberObject::New(Context* ctx, const double& value) {
  JSNumberObject* const obj = new JSNumberObject(value);
  const Class& cls = ctx->Cls("Number");
  obj->set_cls(cls.name);
  obj->set_prototype(cls.prototype);
  return obj;
}

JSNumberObject* JSNumberObject::NewPlain(Context* ctx, const double& value) {
  return new JSNumberObject(value);
}

JSBooleanObject::JSBooleanObject(bool value)
  : JSObject(),
    value_(value) {
}

JSBooleanObject* JSBooleanObject::NewPlain(Context* ctx, bool value) {
  return new JSBooleanObject(value);
}

JSBooleanObject* JSBooleanObject::New(Context* ctx, bool value) {
  JSBooleanObject* const obj = new JSBooleanObject(value);
  const Class& cls = ctx->Cls(ctx->Intern("Boolean"));
  obj->set_cls(cls.name);
  obj->set_prototype(cls.prototype);
  return obj;
}

} }  // namespace iv::lv5
