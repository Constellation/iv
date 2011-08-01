#ifndef _IV_LV5_JSARGUMENTS_H_
#define _IV_LV5_JSARGUMENTS_H_
#include "ast.h"
#include "lv5/symbol.h"
#include "lv5/gc_template.h"
#include "lv5/error.h"
#include "lv5/property.h"
#include "lv5/jsenv.h"
#include "lv5/jsobject.h"
#include "lv5/arguments.h"
#include "lv5/bind.h"
namespace iv {
namespace lv5 {

class Context;
class AstFactory;

class JSArguments : public JSObject {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "Arguments",
      Class::Arguments
    };
    return &cls;
  }
};

class JSNormalArguments : public JSArguments {
 public:
  typedef core::SpaceVector<AstFactory, Identifier*>::type Identifiers;
  typedef GCHashMap<Symbol, Symbol>::type Index2Param;
  JSNormalArguments(Context* ctx, JSDeclEnv* env)
    : env_(env),
      map_() { }

  template<typename Idents, typename ArgsReverseIter>
  static JSNormalArguments* New(Context* ctx,
                                JSFunction* func,
                                const Idents& names,
                                ArgsReverseIter it,
                                ArgsReverseIter last,
                                JSDeclEnv* env,
                                Error* e) {
    JSNormalArguments* const obj = new JSNormalArguments(ctx, env);
    const uint32_t len = std::distance(it, last);
    obj->set_cls(JSArguments::GetClass());
    obj->set_prototype(context::GetClassSlot(ctx, Class::Arguments).prototype);
    bind::Object binder(ctx, obj);
    binder
        .def(symbol::length(),
             JSVal::UInt32(len), bind::W | bind::C);
    SetArguments(ctx, obj, &binder, names, it, last, len);
    binder.def(symbol::callee(), func, bind::W | bind::C);
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
      if (name == symbol::caller() &&
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
  template<typename Idents, typename ArgsReverseIter>
  static void SetArguments(Context* ctx,
                           JSNormalArguments* obj,
                           bind::Object* binder,
                           const Idents& names,
                           ArgsReverseIter it, ArgsReverseIter last,
                           uint32_t len) {
    uint32_t index = len - 1;
    const std::size_t names_len = names.size();
    for (; it != last; ++it) {
      const Symbol sym = symbol::MakeSymbolFromIndex(index);
      binder->def(sym, *it, bind::W | bind::E | bind::C);
      if (index < names_len) {
        obj->map_.insert(
            std::make_pair(sym, GetIdent(names, index)));
      }
      index -= 1;
    }
  }

  static Symbol GetIdent(const Identifiers& names, uint32_t index) {
    return names[index]->symbol();
  }

  template<typename T>
  static Symbol GetIdent(const T& names, uint32_t index) {
    return names[index];
  }

  JSDeclEnv* env_;
  Index2Param map_;
};

// not search environment
class JSStrictArguments : public JSArguments {
 public:
  template<typename ArgsReverseIter>
  static JSStrictArguments* New(Context* ctx,
                                JSFunction* func,
                                ArgsReverseIter it,
                                ArgsReverseIter last,
                                Error* e) {
    JSStrictArguments* const obj = new JSStrictArguments();
    const uint32_t len = std::distance(it, last);
    obj->set_cls(JSArguments::GetClass());
    obj->set_prototype(context::GetClassSlot(ctx, Class::Arguments).prototype);
    bind::Object binder(ctx, obj);
    binder
        .def(symbol::length(),
             JSVal::UInt32(len), bind::W | bind::C);
    uint32_t index = len - 1;
    for (; it != last; ++it, --index) {
      binder.def(symbol::MakeSymbolFromIndex(index),
                 *it, bind::W | bind::E | bind::C);
    }

    JSFunction* const throw_type_error = context::throw_type_error(ctx);
    binder
        .def_accessor(symbol::caller(),
                      throw_type_error,
                      throw_type_error,
                      bind::NONE)
        .def_accessor(symbol::callee(),
                      throw_type_error,
                      throw_type_error,
                      bind::NONE);
    return obj;
  }

  JSVal Get(Context* ctx, Symbol name, Error* e) {
    const JSVal v = JSObject::Get(ctx, name, IV_LV5_ERROR(e));
    if (name == symbol::caller() &&
        v.IsCallable() &&
        v.object()->AsCallable()->IsStrict()) {
      e->Report(Error::Type,
                "access to strict function \"caller\" not allowed");
      return JSUndefined;
    }
    return v;
  }
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSARGUMENTS_H_
