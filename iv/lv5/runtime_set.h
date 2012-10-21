#ifndef IV_LV5_RUNTIME_SET_H_
#define IV_LV5_RUNTIME_SET_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/context.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsset.h>

namespace iv {
namespace lv5 {
namespace runtime {

inline JSVal SetConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  JSSet* set = JSSet::New(ctx);

  const JSVal first = args.At(0);
  if (first.IsUndefined()) {
    return set;
  }

  if (!first.IsObject()) {
    e->Report(Error::Type, "first argument is not iterable");
    return JSEmpty;
  }

  JSObject* const obj = first.object();
  PropertyNamesCollector collector;
  obj->GetOwnPropertyNames(ctx, &collector, EXCLUDE_NOT_ENUMERABLE);
  for (PropertyNamesCollector::Names::const_iterator
       it = collector.names().begin(),
       last = collector.names().end();
       it != last; ++it) {
    const JSVal key = obj->Get(ctx, (*it), IV_LV5_ERROR(e));
    set->AddKey(key);
  }
  return set;
}

inline JSVal SetHas(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Set.prototype.has", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Set>()) {
    JSSet* set = static_cast<JSSet*>(obj.object());
    return JSVal::Bool(set->HasKey(args.At(0)));
  }
  e->Report(Error::Type, "Set.prototype.has is not generic function");
  return JSUndefined;
}

inline JSVal SetAdd(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Set.prototype.add", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Set>()) {
    JSSet* set = static_cast<JSSet*>(obj.object());
    set->AddKey(args.At(0));
    return JSUndefined;
  }
  e->Report(Error::Type, "Set.prototype.add is not generic function");
  return JSUndefined;
}

inline JSVal SetDelete(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Set.prototype.delete", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Set>()) {
    JSSet* set = static_cast<JSSet*>(obj.object());
    return JSVal::Bool(set->DeleteKey(args.At(0)));
  }
  e->Report(Error::Type, "Set.prototype.delete is not generic function");
  return JSUndefined;
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_SET_H_
