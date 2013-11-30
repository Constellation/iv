// This is ES.next Set implementation
//
// detail
// http://wiki.ecmascript.org/doku.php?id=harmony:simple_maps_and_sets
//
#ifndef IV_LV5_JSSET_H_
#define IV_LV5_JSSET_H_
#include <iv/all_static.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class Context;

class JSSet : public core::AllStatic {
 public:

  class Data : public radio::HeapObject<> {
   public:
    typedef GCHashSet<JSVal, JSVal::Hasher, JSVal::SameValueEqualer>::type Set;
    Data() : data_() { }

    bool Delete(JSVal val) {
      Set::const_iterator it = data_.find(val);
      if (it != data_.end()) {
        data_.erase(it);
        return true;
      }
      return false;
    }

    bool Has(JSVal val) const {
      return data_.find(val) != data_.end();
    }

    void Add(JSVal key) {
      data_.insert(key);
    }

    void Clear() {
      data_.clear();
    }

    const Set& set() const { return data_; }

   private:
    Set data_;
  };

  static JSObject* Initialize(Context* ctx, JSVal input, JSVal it, Error* e) {
    if (!input.IsObject()) {
      e->Report(Error::Type, "SetInitialize to non-object");
      return nullptr;
    }

    JSObject* obj = input.object();
    if (obj->HasOwnProperty(ctx, symbol())) {
      e->Report(Error::Type, "re-initialize map object");
      return nullptr;
    }

    if (!obj->IsExtensible()) {
      e->Report(Error::Type, "SetInitialize to un-extensible object");
      return nullptr;
    }

    JSObject* iterable = nullptr;
    JSFunction* adder = nullptr;
    if (!it.IsUndefined()) {
      iterable = it.ToObject(ctx, IV_LV5_ERROR(e));
      JSVal val = obj->Get(ctx, symbol::add(), IV_LV5_ERROR(e));
      if (!val.IsCallable()) {
        e->Report(Error::Type, "SetInitialize adder, `obj.add` is not callable");
        return nullptr;
      }
      adder = static_cast<JSFunction*>(val.object());
    }

    Data* data = new Data;

    obj->DefineOwnProperty(
      ctx,
      symbol(),
      DataDescriptor(JSVal::Cell(data), ATTR::W | ATTR::E | ATTR::C),
      false, IV_LV5_ERROR(e));

    if (iterable) {
      // TODO(Constellation) iv / lv5 doesn't have iterator system
      PropertyNamesCollector collector;
      iterable->GetOwnPropertyNames(ctx, &collector, EXCLUDE_NOT_ENUMERABLE);
      for (PropertyNamesCollector::Names::const_iterator
           it = collector.names().begin(),
           last = collector.names().end();
           it != last; ++it) {
        const JSVal value = iterable->Get(ctx, (*it), IV_LV5_ERROR(e));
        ScopedArguments arg_list(ctx, 1, IV_LV5_ERROR(e));
        arg_list[0] = value;
        adder->Call(&arg_list, obj, IV_LV5_ERROR(e));
      }
    }

    return obj;
  }

  static Symbol symbol() {
    static const core::UString kSetData = core::ToUString("[[SetData]]");
    return core::detail::MakePrivateSymbol(&kSetData);
  }
};



} }  // namespace iv::lv5
#endif  // IV_LV5_JSSET_H_
