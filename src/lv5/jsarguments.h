#ifndef _IV_LV5_JSARGUMENTS_H_
#define _IV_LV5_JSARGUMENTS_H_
#include "ast.h"
#include "lv5/symbol.h"
#include "lv5/gc_template.h"
#include "lv5/property.h"
#include "lv5/jsenv.h"
#include "lv5/jsobject.h"
#include "lv5/arguments.h"
#include "lv5/bind.h"
namespace iv {
namespace lv5 {

class Context;
class JSObject;
class Error;
class JSDeclEnv;
class AstFactory;

class JSArguments : public JSObject {
 public:
  typedef core::SpaceVector<AstFactory, Identifier*>::type Identifiers;
  typedef GCHashMap<Symbol, Symbol>::type Index2Param;
  JSArguments(Context* ctx, JSDeclEnv* env)
    : env_(env),
      map_() { }

  static JSArguments* New(Context* ctx,
                          JSFunction* func,
                          const Identifiers& names,
                          const Arguments& args,
                          JSDeclEnv* env,
                          bool strict,
                          Error* e) {
    JSArguments* const obj = new JSArguments(ctx, env);
    const uint32_t len = args.size();
    const std::size_t names_len = names.size();
    const Class& cls = context::Cls(ctx, "Arguments");
    bind::Object binder(ctx, obj);
    binder
        .class_name(cls.name)
        .prototype(cls.prototype)
        .def(context::length_symbol(ctx),
             JSVal::UInt32(len), bind::W | bind::C);

    uint32_t index = len - 1;
    for (Arguments::const_reverse_iterator it = args.rbegin(),
         last = args.rend(); it != last; ++it) {
      const Symbol sym = context::Intern(ctx, index);
      binder.def(sym, *it, bind::W | bind::E | bind::C);
      if (index < names_len) {
        if (!strict) {
          obj->map_.insert(
              std::make_pair(sym, names[index]->symbol()));
        }
      }
      index -= 1;
    }
    if (strict) {
      JSFunction* const throw_type_error = context::throw_type_error(ctx);
      binder
          .def_accessor(context::caller_symbol(ctx),
                        throw_type_error,
                        throw_type_error,
                        bind::NONE)
          .def_accessor(context::callee_symbol(ctx),
                        throw_type_error,
                        throw_type_error,
                        bind::NONE);
    } else {
      binder.def(context::callee_symbol(ctx), func, bind::W | bind::C);
    }
    return obj;
  }

  JSVal Get(Context* ctx, Symbol name, Error* e) {
    const Index2Param::const_iterator it = map_.find(name);
    if (it != map_.end()) {
      return env_->GetBindingValue(ctx, it->second, true, e);
    } else {
      const JSVal v = JSObject::Get(ctx, name, e);
      if (*e) {
        return JSUndefined;
      }
      if (name == context::caller_symbol(ctx) &&
          v.IsCallable() &&
          v.object()->AsCallable()->IsStrict()) {
        e->Report(Error::Type,
                  "access to strict function \"caller\" not allowed");
        return JSUndefined;
      }
      return v;
    }
  }

  PropertyDescriptor GetOwnProperty(Context* ctx, Symbol name) const {
    const PropertyDescriptor desc = JSObject::GetOwnProperty(ctx, name);
    if (desc.IsEmpty()) {
      return desc;
    }
    const Index2Param::const_iterator it = map_.find(name);
    if (it != map_.end()) {
      const JSVal val = env_->GetBindingValue(it->second);
      return DataDescriptor(val,
                            desc.attrs() & PropertyDescriptor::kDataAttrField);
    }
    return desc;
  }

  bool DefineOwnProperty(Context* ctx, Symbol name,
                         const PropertyDescriptor& desc,
                         bool th, Error* e) {
    const bool allowed = JSObject::DefineOwnProperty(ctx, name,
                                                     desc, false, e);
    if (!allowed) {
      if (th) {
        e->Report(Error::Type,
                  "[[DefineOwnProperty]] failed");
      }
      return false;
    }
    const Index2Param::const_iterator it = map_.find(name);
    if (it != map_.end()) {
      if (desc.IsAccessorDescriptor()) {
        map_.erase(it);
      } else {
        if (desc.IsDataDescriptor()) {
          const DataDescriptor* const data = desc.AsDataDescriptor();
          if (!data->IsValueAbsent()) {
            env_->SetMutableBinding(ctx,
                                    it->second, data->value(),
                                    th, e);
            if (*e) {
              return false;
            }
          }
          if (!data->IsWritableAbsent() && !data->IsWritable()) {
            map_.erase(it);
          }
        }
      }
    }
    return true;
  }

  bool Delete(Context* ctx, Symbol name, bool th, Error* e) {
    const bool result = JSObject::Delete(ctx, name, th, e);
    if (*e) {
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

 private:
  JSDeclEnv* env_;
  Index2Param map_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARGUMENTS_H_
