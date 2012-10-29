// This is ES.next Map implementation
//
// detail
// http://wiki.ecmascript.org/doku.php?id=harmony:simple_maps_and_sets
//
#ifndef IV_LV5_JSMAP_H_
#define IV_LV5_JSMAP_H_
#include <iv/ustring.h>
#include <iv/all_static.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class Context;

class JSMap : public core::AllStatic {
 public:

  class Data : public radio::HeapObject<> {
   public:
    typedef GCHashMap<JSVal, JSVal,
                      JSVal::Hasher, JSVal::SameValueEqualer>::type Mapping;
    Data() : data_() { }

    bool Delete(JSVal val) {
      Mapping::const_iterator it = data_.find(val);
      if (it != data_.end()) {
        data_.erase(it);
        return true;
      }
      return false;
    }

    JSVal Get(JSVal val) const {
      Mapping::const_iterator it = data_.find(val);
      if (it != data_.end()) {
        return it->second;
      }
      return JSUndefined;
    }

    bool Has(JSVal val) const {
      return data_.find(val) != data_.end();
    }

    void Set(JSVal key, JSVal val) {
      data_[key] = val;
    }

    void Clear() {
      data_.clear();
    }

    const Mapping& mapping() const { return data_; }

   private:
    Mapping data_;
  };

  static JSObject* Initialize(Context* ctx, JSVal input, JSVal it, Error* e) {
    if (!input.IsObject()) {
      e->Report(Error::Type, "MapInitialize to non-object");
      return NULL;
    }

    JSObject* obj = input.object();
    if (obj->HasOwnProperty(ctx, symbol())) {
      e->Report(Error::Type, "re-initialize map object");
      return NULL;
    }

    if (!obj->IsExtensible()) {
      e->Report(Error::Type, "MapInitialize to un-extensible object");
      return NULL;
    }

    JSObject* iterable = NULL;
    JSFunction* adder = NULL;
    if (!it.IsUndefined()) {
      iterable = it.ToObject(ctx, IV_LV5_ERROR_WITH(e, NULL));
      JSVal val = obj->Get(ctx, symbol::set(), IV_LV5_ERROR_WITH(e, NULL));
      if (!val.IsCallable()) {
        e->Report(Error::Type, "MapInitialize adder, `obj.set` is not callable");
        return NULL;
      }
      adder = static_cast<JSFunction*>(val.object());
    }

    Data* data = new Data;

    obj->DefineOwnProperty(
      ctx,
      symbol(),
      DataDescriptor(JSVal::Cell(data), ATTR::W | ATTR::E | ATTR::C),
      false, IV_LV5_ERROR_WITH(e, NULL));

    if (iterable) {
      // TODO(Constellation) iv / lv5 doesn't have iterator system
      PropertyNamesCollector collector;
      iterable->GetOwnPropertyNames(ctx, &collector, EXCLUDE_NOT_ENUMERABLE);
      for (PropertyNamesCollector::Names::const_iterator
           it = collector.names().begin(),
           last = collector.names().end();
           it != last; ++it) {
        const JSVal v = iterable->Get(ctx, (*it), IV_LV5_ERROR_WITH(e, NULL));
        JSObject* item = v.ToObject(ctx, IV_LV5_ERROR_WITH(e, NULL));
        const JSVal key =
            item->Get(ctx, symbol::MakeSymbolFromIndex(0), IV_LV5_ERROR_WITH(e, NULL));
        const JSVal value =
            item->Get(ctx, symbol::MakeSymbolFromIndex(1), IV_LV5_ERROR_WITH(e, NULL));
        ScopedArguments arg_list(ctx, 2, IV_LV5_ERROR_WITH(e, NULL));
        arg_list[0] = key;
        arg_list[1] = value;
        adder->Call(&arg_list, obj, IV_LV5_ERROR_WITH(e, NULL));
      }
    }

    return obj;
  }

  static Symbol symbol() {
    static const core::UString kMapData = core::ToUString("[[MapData]]");
    return core::detail::MakePrivateSymbol(&kMapData);
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSMAP_H_
