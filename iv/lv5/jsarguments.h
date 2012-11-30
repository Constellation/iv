#ifndef IV_LV5_JSARGUMENTS_H_
#define IV_LV5_JSARGUMENTS_H_
#include <iv/ast.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/error.h>
#include <iv/lv5/property.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/map.h>
#include <iv/lv5/slot.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/bind.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/context.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {

class Context;
class AstFactory;

// only class placeholder
class JSArguments : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSArguments, Arguments)

  explicit JSArguments(Map* map) : JSObject(map) { set_cls(GetClass()); }

  enum {
    FIELD_LENGTH = 0,
    FIELD_CALLEE = 1,
    FIELD_CALLER = 2  // only in strict arguments
  };
};

class JSNormalArguments : public JSArguments {
 public:
  typedef GCVector<Symbol>::type Indice;

  template<typename Idents, typename ArgsReverseIter>
  static JSNormalArguments* New(Context* ctx,
                                JSFunction* func,
                                const Idents& names,
                                ArgsReverseIter it,
                                ArgsReverseIter last,
                                JSDeclEnv* env,
                                Error* e) {
    JSNormalArguments* const obj = new JSNormalArguments(ctx, env);
    const uint32_t len = static_cast<uint32_t>(std::distance(it, last));
    obj->Direct(FIELD_LENGTH) = JSVal::UInt32(len);
    obj->Direct(FIELD_CALLEE) = func;
    bind::Object binder(ctx, obj);
    obj->SetArguments(ctx, &binder, names, it, last, len);
    return obj;
  }

  virtual JSVal GetSlot(Context* ctx, Symbol name, Slot* slot, Error* e) {
    const JSVal v = JSObject::GetSlot(ctx, name, slot, IV_LV5_ERROR(e));
    if (name == symbol::caller() &&
        v.IsCallable() &&
        static_cast<JSFunction*>(v.object())->strict()) {
      e->Report(Error::Type,
                "access to strict function \"caller\" not allowed");
      return JSUndefined;
    }
    return v;
  }

  virtual bool GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
    if (symbol::IsArrayIndexSymbol(name)) {
      return JSObject::GetOwnPropertySlot(ctx, name, slot);
    }

    const uint32_t index = symbol::GetIndexFromSymbol(name);
    const bool result = JSObject::GetOwnIndexedPropertySlotInternal(ctx, index, slot);
    if (mapping_.size() > index) {
      const Symbol mapped = mapping_[index];
      if (mapped != symbol::kDummySymbol) {
        Error::Dummy dummy;
        const JSVal val = env_->GetBindingValue(ctx, mapped, false, &dummy);
        slot->set(val, slot->attributes(), this);
        return true;
      }
    }
    return true;
  }

  virtual bool DefineOwnPropertySlot(Context* ctx, Symbol name,
                                     const PropertyDescriptor& desc,
                                     Slot* slot,
                                     bool th, Error* e) {
    if (!symbol::IsArrayIndexSymbol(name)) {
      return JSObject::DefineOwnPropertySlot(ctx, name, desc, slot, th, e);
    }

    const uint32_t index = symbol::GetIndexFromSymbol(name);
    if (!DefineOwnIndexedPropertyInternal(ctx, index, desc, false, e)) {
      if (th) {
        e->Report(Error::Type, "[[DefineOwnProperty]] failed");
      }
      return false;
    }

    if (symbol::IsArrayIndexSymbol(name)) {
      if (mapping_.size() > index) {
        const Symbol mapped = mapping_[index];
        bool dummy = false;
        if (mapped != symbol::kDummySymbol) {
          if (desc.IsAccessor()) {
            mapping_[index] = symbol::kDummySymbol;
            dummy = true;
          } else {
            if (desc.IsData()) {
              const DataDescriptor* const data = desc.AsDataDescriptor();
              if (!data->IsValueAbsent()) {
                env_->SetMutableBinding(ctx, mapped, data->value(),
                                        th, IV_LV5_ERROR_WITH(e, false));
              }
              if (!data->IsWritableAbsent() && !data->IsWritable()) {
                mapping_[index] = symbol::kDummySymbol;
                dummy = true;
              }
            }
          }

          if (!dummy) {
            // make store cache off
            slot->MakePutUnCacheable();
          }
        }
      }
    }
    return true;
  }

  virtual bool Delete(Context* ctx, Symbol name, bool th, Error* e) {
    if (!symbol::IsArrayIndexSymbol(name)) {
      return JSObject::Delete(ctx, name, th, e);
    }
    const uint32_t index = symbol::GetIndexFromSymbol(name);
    const bool result =
        JSObject::DeleteIndexedInternal(ctx, index, th, IV_LV5_ERROR_WITH(e, false));
    if (mapping_.size() > index) {
      const Symbol mapped = mapping_[index];
      if (mapped != symbol::kDummySymbol) {
        mapping_[index] = symbol::kDummySymbol;
        return true;
      }
    }
    return result;
  }

  void MarkChildren(radio::Core* core) {
    JSObject::MarkChildren(core);
    core->MarkCell(env_);
  }

 private:
  JSNormalArguments(Context* ctx, JSDeclEnv* env)
    : JSArguments(ctx->global_data()->normal_arguments_map()),
      env_(env),
      mapping_() { }

  template<typename Idents, typename ArgsReverseIter>
  void SetArguments(Context* ctx,
                    bind::Object* binder,
                    const Idents& names,
                    ArgsReverseIter it, ArgsReverseIter last,
                    uint32_t len) {
    uint32_t index = len - 1;
    const uint32_t names_len = static_cast<uint32_t>(names.size());
    mapping_.resize((std::min)(len, names_len), symbol::kDummySymbol);
    for (; it != last; ++it) {
      binder->def(symbol::MakeSymbolFromIndex(index),
                  *it, ATTR::W | ATTR::E | ATTR::C);
      if (index < names_len) {
        const Symbol name = GetSymbol(names, index);
        if (std::find(mapping_.begin() + index + 1,
                      mapping_.end(), name) == mapping_.end()) {
          mapping_[index] = name;
        }
      }
      index -= 1;
    }
  }

  template<typename Ident>
  static Symbol GetSymbol(const Ident& ident, uint32_t index) {
    return ident[index];
  }

  static Symbol GetSymbol(const Assigneds& ident, uint32_t index) {
    return ident[index]->symbol();
  }

  JSDeclEnv* env_;
  Indice mapping_;
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
    JSStrictArguments* const obj = new JSStrictArguments(ctx);
    const uint32_t len = static_cast<uint32_t>(std::distance(it, last));
    JSFunction* throw_type_error = ctx->throw_type_error();
    obj->Direct(FIELD_LENGTH) = JSVal::UInt32(len);
    obj->Direct(FIELD_CALLER) = JSVal::Cell(Accessor::New(ctx, throw_type_error, throw_type_error));
    obj->Direct(FIELD_CALLEE) = JSVal::Cell(Accessor::New(ctx, throw_type_error, throw_type_error));
    uint32_t index = len - 1;
    bind::Object binder(ctx, obj);
    for (; it != last; ++it, --index) {
      binder.def(symbol::MakeSymbolFromIndex(index),
                 *it, ATTR::W | ATTR::E | ATTR::C);
    }
    return obj;
  }

  virtual JSVal GetSlot(Context* ctx, Symbol name, Slot* slot, Error* e) {
    const JSVal v = JSObject::GetSlot(ctx, name, slot, IV_LV5_ERROR(e));
    if (name == symbol::caller() &&
        v.IsCallable() &&
        static_cast<JSFunction*>(v.object())->strict()) {
      e->Report(Error::Type,
                "access to strict function \"caller\" not allowed");
      return JSUndefined;
    }
    return v;
  }

 private:
  explicit JSStrictArguments(Context* ctx)
    : JSArguments(ctx->global_data()->strict_arguments_map()) { }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSARGUMENTS_H_
