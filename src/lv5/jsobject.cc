#include <cassert>
#include <algorithm>
#include "lv5/jsobject.h"
#include "lv5/context_utils.h"
#include "lv5/property.h"
#include "lv5/jsfunction.h"
#include "lv5/jsval.h"
#include "lv5/jsenv.h"
#include "lv5/context.h"
#include "lv5/class.h"
#include "lv5/object_utils.h"
#include "lv5/jsbooleanobject.h"
#include "lv5/jsnumberobject.h"
#include "lv5/jshashobject.h"
#include "lv5/error_check.h"

namespace iv {
namespace lv5 {

JSObject::JSObject()
  : cls_(NULL),
    prototype_(NULL),
    extensible_(true),
    table_(NULL) {
}

JSObject::JSObject(JSObject* proto,
                   Class* cls,
                   bool extensible)
  : cls_(cls),
    prototype_(proto),
    extensible_(extensible),
    table_(NULL) {
}

JSObject::JSObject(Properties* table)
  : cls_(NULL),
    prototype_(NULL),
    extensible_(true),
    table_(table) {
}

#define TRY(context, sym, arg, error)\
  do {\
    const JSVal method = Get(context, sym, error);\
    if (*error) {\
      return JSUndefined;\
    }\
    if (method.IsCallable()) {\
      const JSVal val = method.object()->AsCallable()->Call(&arg, this, error);\
      if (*error) {\
        return JSUndefined;\
      }\
      if (val.IsPrimitive() || val.IsNull() || val.IsUndefined()) {\
        return val;\
      }\
    }\
  } while (0)
JSVal JSObject::DefaultValue(Context* ctx,
                             Hint::Object hint, Error* e) {
  ScopedArguments args(ctx, 0, IV_LV5_ERROR(e));
  if (hint == Hint::STRING) {
    // hint is STRING
    TRY(ctx, symbol::toString(), args, e);
    TRY(ctx, symbol::valueOf(), args, e);
  } else {
    // section 8.12.8
    // hint is NUMBER or NONE
    TRY(ctx, symbol::valueOf(), args, e);
    TRY(ctx, symbol::toString(), args, e);
  }
  e->Report(Error::Type, "invalid default value");
  return JSUndefined;
}
#undef TRY

JSVal JSObject::Get(Context* ctx,
                    Symbol name, Error* e) {
  const PropertyDescriptor desc = GetProperty(ctx, name);
  if (desc.IsEmpty()) {
    return JSUndefined;
  }
  if (desc.IsDataDescriptor()) {
    return desc.AsDataDescriptor()->value();
  } else {
    assert(desc.IsAccessorDescriptor());
    JSObject* const getter = desc.AsAccessorDescriptor()->get();
    if (getter) {
      ScopedArguments a(ctx, 0, IV_LV5_ERROR(e));
      return getter->AsCallable()->Call(&a, this, e);
    } else {
      return JSUndefined;
    }
  }
}

// not recursion
PropertyDescriptor JSObject::GetProperty(Context* ctx, Symbol name) const {
  const JSObject* obj = this;
  do {
    const PropertyDescriptor prop = obj->GetOwnProperty(ctx, name);
    if (!prop.IsEmpty()) {
      return prop;
    }
    obj = obj->prototype();
  } while (obj);
  return JSUndefined;
}

PropertyDescriptor JSObject::GetOwnProperty(Context* ctx, Symbol name) const {
  if (table_) {
    const Properties::const_iterator it = table_->find(name);
    if (it == table_->end()) {
      return JSUndefined;
    } else {
      return it->second;
    }
  } else {
    return JSUndefined;
  }
}

bool JSObject::CanPut(Context* ctx, Symbol name) const {
  const PropertyDescriptor desc = GetOwnProperty(ctx, name);
  if (!desc.IsEmpty()) {
    if (desc.IsAccessorDescriptor()) {
      return desc.AsAccessorDescriptor()->set();
    } else {
      assert(desc.IsDataDescriptor());
      return desc.AsDataDescriptor()->IsWritable();
    }
  }
  if (!prototype_) {
    return extensible_;
  }
  const PropertyDescriptor inherited = prototype_->GetProperty(ctx, name);
  if (inherited.IsEmpty()) {
    return extensible_;
  } else {
    if (inherited.IsAccessorDescriptor()) {
      return inherited.AsAccessorDescriptor()->set();
    } else {
      assert(inherited.IsDataDescriptor());
      return inherited.AsDataDescriptor()->IsWritable();
    }
  }
}

#define REJECT(str)\
  do {\
    if (th) {\
      e->Report(Error::Type, str);\
    }\
    return false;\
  } while (0)

bool JSObject::DefineOwnProperty(Context* ctx,
                                 Symbol name,
                                 const PropertyDescriptor& desc,
                                 bool th,
                                 Error* e) {
  // section 8.12.9 [[DefineOwnProperty]]
  const PropertyDescriptor current = GetOwnProperty(ctx, name);

  // empty check
  if (current.IsEmpty()) {
    if (!extensible_) {
      REJECT("object not extensible");
    } else {
      AllocateTable();
      if (!desc.IsAccessorDescriptor()) {
        assert(desc.IsDataDescriptor() || desc.IsGenericDescriptor());
        (*table_)[name] = PropertyDescriptor::SetDefault(desc);
      } else {
        assert(desc.IsAccessorDescriptor());
        (*table_)[name] = PropertyDescriptor::SetDefault(desc);
      }
      return true;
    }
  }
  bool returned = false;
  if (IsDefineOwnPropertyAccepted(current, desc, th, &returned, e)) {
    AllocateTable();
    (*table_)[name] = PropertyDescriptor::Merge(desc, current);
  }
  return returned;
}

#undef REJECT

void JSObject::Put(Context* ctx,
                   Symbol name,
                   const JSVal& val, bool th, Error* e) {
  if (!CanPut(ctx, name)) {
    if (th) {
      e->Report(Error::Type, "put failed");
    }
    return;
  }
  const PropertyDescriptor own_desc = GetOwnProperty(ctx, name);
  if (!own_desc.IsEmpty() && own_desc.IsDataDescriptor()) {
    DefineOwnProperty(ctx,
                      name,
                      DataDescriptor(
                          val,
                          PropertyDescriptor::UNDEF_ENUMERABLE |
                          PropertyDescriptor::UNDEF_CONFIGURABLE |
                          PropertyDescriptor::UNDEF_WRITABLE), th, e);
    return;
  }
  const PropertyDescriptor desc = GetProperty(ctx, name);
  if (!desc.IsEmpty() && desc.IsAccessorDescriptor()) {
    const AccessorDescriptor* const accs = desc.AsAccessorDescriptor();
    assert(accs->set());
    ScopedArguments args(ctx, 1, IV_LV5_ERROR_VOID(e));
    args[0] = val;
    accs->set()->AsCallable()->Call(&args, this, e);
  } else {
    DefineOwnProperty(ctx, name,
                      DataDescriptor(val,
                                     PropertyDescriptor::WRITABLE |
                                     PropertyDescriptor::ENUMERABLE |
                                     PropertyDescriptor::CONFIGURABLE),
                      th, e);
  }
}

bool JSObject::HasProperty(Context* ctx, Symbol name) const {
  return !GetProperty(ctx, name).IsEmpty();
}

bool JSObject::Delete(Context* ctx, Symbol name, bool th, Error* e) {
  if (table_) {
    const PropertyDescriptor desc = GetOwnProperty(ctx, name);
    if (desc.IsEmpty()) {
      return true;
    }
    if (desc.IsConfigurable()) {
      table_->erase(name);
      return true;
    }
    if (th) {
      e->Report(Error::Type, "delete failed");
    }
    return false;
  } else {
    return true;
  }
}

void JSObject::GetPropertyNames(Context* ctx,
                                std::vector<Symbol>* vec,
                                EnumerationMode mode) const {
  GetOwnPropertyNames(ctx, vec, mode);
  const JSObject* obj = prototype_;
  while (obj) {
    obj->GetOwnPropertyNames(ctx, vec, mode);
    obj = obj->prototype();
  }
}

void JSObject::GetOwnPropertyNames(Context* ctx,
                                   std::vector<Symbol>* vec,
                                   EnumerationMode mode) const {
  if (table_) {
    if (vec->empty()) {
      for (JSObject::Properties::const_iterator it = table_->begin(),
           last = table_->end(); it != last; ++it) {
        if (it->second.IsEnumerable() || (mode == kIncludeNotEnumerable)) {
          vec->push_back(it->first);
        }
      }
    } else {
      for (JSObject::Properties::const_iterator it = table_->begin(),
           last = table_->end(); it != last; ++it) {
        if ((it->second.IsEnumerable() || (mode == kIncludeNotEnumerable)) &&
            (std::find(vec->begin(), vec->end(), it->first) == vec->end())) {
          vec->push_back(it->first);
        }
      }
    }
  }
}

JSObject* JSObject::New(Context* ctx) {
  return JSHashObject::New(ctx);
}

JSObject* JSObject::NewPlain(Context* ctx) {
  return JSHashObject::NewPlain(ctx);
}

} }  // namespace iv::lv5
