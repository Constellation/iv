#include <cassert>
#include <vector>
#include "jsobject.h"
#include "jsproperty.h"
#include "jsfunction.h"
#include "jsval.h"
#include "jsenv.h"
#include "interpreter.h"
#include "context.h"
#include "ustream.h"
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
    JSVal method = Get(context, context->Intern(sym), error);\
    if (*error) {\
      return JSVal::Undefined();\
    }\
    if (method.IsCallable()) {\
      JSVal val = method.object()->AsCallable()->Call(arg, error);\
      if (*error) {\
        return JSVal::Undefined();\
      }\
      if (val.IsPrimitive()) {\
        return val;\
      }\
    }\
  } while (0)
JSVal JSObject::DefaultValue(Context* ctx,
                             Hint hint, JSErrorCode::Type* res) {
  const Arguments args(ctx, JSVal(this));
  if (hint != NUMBER) {
    // hint is STRING or NONE
    TRY(ctx, "toString", args, res);
    TRY(ctx, "valueOf", args, res);
  } else {
    TRY(ctx, "valueOf", args, res);
    TRY(ctx, "toString", args, res);
  }
  *res = JSErrorCode::TypeError;
  return JSVal::Undefined();
}
#undef TRY

JSVal JSObject::Get(Context* ctx,
                    Symbol name, JSErrorCode::Type* res) {
  PropertyDescriptor* desc = GetProperty(name);
  if (!desc) {
    return JSVal::Undefined();
  }
  DataDescriptor* data = desc->AsDataDescriptor();
  if (data) {
    return data->value();
  } else {
    assert(desc->IsAccessorDescriptor());
    JSObject* getter = desc->AsAccessorDescriptor()->get();
    if (getter) {
      return getter->AsCallable()->Call(Arguments(ctx, JSVal(this)), res);
    } else {
      return JSVal::Undefined();
    }
  }
}

// not recursion
PropertyDescriptor* JSObject::GetProperty(Symbol name) const {
  const JSObject* obj = this;
  do {
    PropertyDescriptor* prop = obj->GetOwnProperty(name);
    if (prop) {
      return prop;
    }
    obj = obj->prototype();
  } while (obj);
  return NULL;
}

PropertyDescriptor* JSObject::GetOwnProperty(Symbol name) const {
  const Properties::const_iterator it = table_.find(name);
  if (it == table_.end()) {
    return NULL;
  } else {
    return it->second->clone();
  }
}

bool JSObject::CanPut(Symbol name) const {
  PropertyDescriptor* desc = GetOwnProperty(name);
  if (desc) {
    AccessorDescriptor* accs = desc->AsAccessorDescriptor();
    if (accs) {
      return accs->set();
    } else {
      assert(desc->IsDataDescriptor());
      return desc->IsWritable();
    }
  }
  if (!prototype_) {
    return extensible_;
  }
  PropertyDescriptor* inherited = prototype_->GetProperty(name);
  if (!inherited) {
    return extensible_;
  } else {
    AccessorDescriptor* accs = inherited->AsAccessorDescriptor();
    if (accs) {
      return accs->set();
    } else {
      assert(inherited->IsDataDescriptor());
      return inherited->IsWritable();
    }
  }
}

#define REJECT()\
  do {\
    if (th) {\
      *res = JSErrorCode::TypeError;\
    }\
    return false;\
  } while (0)

bool JSObject::DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor* desc,
                                 bool th,
                                 JSErrorCode::Type* res) {
  // section 8.12.9 [[DefineOwnProperty]]
  PropertyDescriptor* current = GetOwnProperty(name);
  if (!current) {
    if (!extensible_) {
      REJECT();
    } else {
      if (desc->IsDataDescriptor()) {
        table_[name] = desc->clone()->SetDefaultToAbsent();
      } else {
        assert(desc->IsAccessorDescriptor());
        table_[name] = desc->clone()->SetDefaultToAbsent();
      }
      return true;
    }
  }

  // step 5 : this interpreter not allows absent descriptor
  // step 6
  if (desc->Equals(*current)) {
    return true;
  }

  // step 7
  if (current->IsConfigurable()) {
    if (desc->Configurable() == PropertyDescriptor::kTRUE) {
      REJECT();
    }
    if (!desc->IsEnumerableAbsent() &&
        current->Enumerable() != desc->Enumerable()) {
      REJECT();
    }
  }

  // step 9
  if (current->type() != desc->type()) {
    if (!current->Configurable()) {
      REJECT();
    }
    if (current->IsDataDescriptor()) {
      assert(desc->IsAccessorDescriptor());
    } else {
      assert(desc->IsDataDescriptor());
    }
  } else {
    // step 10
    if (current->IsDataDescriptor()) {
      assert(desc->IsDataDescriptor());
      if (current->IsConfigurable()) {
        if (!current->IsWritable()) {
          if (desc->Writable() == PropertyDescriptor::kTRUE) {
            REJECT();
          }
          if (Interpreter::SameValue(current->AsDataDescriptor()->value(),
                                     desc->AsDataDescriptor()->value())) {
            REJECT();
          }
        }
      }
    } else {
      // step 11
      assert(desc->IsAccessorDescriptor());
      if (!current->Configurable()) {
        const AccessorDescriptor* const lhs = current->AsAccessorDescriptor();
        const AccessorDescriptor* const rhs = desc->AsAccessorDescriptor();
        if (lhs->set() != rhs->set() || lhs->get() != rhs->get()) {
          REJECT();
        }
      }
    }
  }
  table_[name] = desc->clone()->MergeAttrs(current->attrs());
  return true;
}

#undef REJECT

void JSObject::Put(Context* ctx,
                   Symbol name,
                   const JSVal& val, bool th, JSErrorCode::Type* res) {
  if (!CanPut(name)) {
    if (th) {
      *res = JSErrorCode::TypeError;
    }
    return;
  }
  PropertyDescriptor* own_desc = GetOwnProperty(name);
  if (own_desc && own_desc->IsDataDescriptor()) {
    DataDescriptor* desc = new DataDescriptor(val);
    DefineOwnProperty(ctx, name, desc, th, res);
    return;
  }
  PropertyDescriptor* desc = GetProperty(name);
  if (desc && desc->IsAccessorDescriptor()) {
    AccessorDescriptor* accs = desc->AsAccessorDescriptor();
    assert(accs->set());
    accs->set()->AsCallable()->Call(Arguments(ctx, JSVal(this)), res);
  } else {
    DataDescriptor* new_desc = new DataDescriptor(
        val,
        PropertyDescriptor::WRITABLE |
        PropertyDescriptor::ENUMERABLE |
        PropertyDescriptor::CONFIGURABLE);
    DefineOwnProperty(ctx, name, new_desc, th, res);
  }
}

bool JSObject::HasProperty(Symbol name) const {
  return GetProperty(name);
}

bool JSObject::Delete(Symbol name, bool th, JSErrorCode::Type* res) {
  PropertyDescriptor* desc = GetOwnProperty(name);
  if (!desc) {
    return true;
  }
  if (desc->IsConfigurable()) {
    table_.erase(name);
    return true;
  } else {
    if (th) {
      *res = JSErrorCode::TypeError;
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
  const Symbol name = ctx->Intern("Object");
  const Class& cls = ctx->Cls(name);
  obj->set_cls(JSString::NewAsciiString(ctx, "String"));
  obj->set_prototype(cls.prototype);
  return obj;
}

JSNumberObject* JSNumberObject::New(Context* ctx, const double& value) {
  JSNumberObject* const obj = new JSNumberObject(value);
  const Symbol name = ctx->Intern("Object");
  const Class& cls = ctx->Cls(name);
  obj->set_cls(JSString::NewAsciiString(ctx, "Number"));
  obj->set_prototype(cls.prototype);
  return obj;
}

JSBooleanObject::JSBooleanObject(bool value)
  : JSObject(),
    value_(value) {
}

JSBooleanObject* JSBooleanObject::New(Context* ctx, bool value) {
  JSBooleanObject* const obj = new JSBooleanObject(value);
  const Symbol name = ctx->Intern("Object");
  const Class& cls = ctx->Cls(name);
  obj->set_cls(JSString::NewAsciiString(ctx, "Boolean"));
  obj->set_prototype(cls.prototype);
  return obj;
}

} }  // namespace iv::lv5
