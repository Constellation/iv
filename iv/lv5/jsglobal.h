#ifndef IV_LV5_JSGLOBAL_H_
#define IV_LV5_JSGLOBAL_H_
#include <iv/notfound.h>
#include <iv/segmented_vector.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/map.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/class.h>
#include <iv/lv5/slot.h>
namespace iv {
namespace lv5 {

class JSGlobal : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSGlobal, global)

  typedef GCHashMap<Symbol, uint32_t>::type SymbolMap;
  typedef core::SegmentedVector<StoredSlot, 8, gc_allocator<StoredSlot> > Vector;

  static JSGlobal* New(Context* ctx) { return new JSGlobal(ctx); }

  typedef std::pair<bool, JSVal> Constant;
  static Constant LookupConstant(Symbol name) {
    if (name == symbol::undefined()) {
      return std::make_pair(true, JSUndefined);
    } else if (name == symbol::NaN()) {
      return std::make_pair(true, JSNaN);
    } else if (name == symbol::Infinity()) {
      return std::make_pair(true, iv::core::math::kInfinity);
    }
    return std::make_pair(false, JSEmpty);
  }

  inline uint32_t LookupVariable(Symbol name) const {
    const SymbolMap::const_iterator it = symbol_map_.find(name);
    if (it != symbol_map_.end()) {
      return it->second;
    }
    return core::kNotFound32;
  }

  void PushVariable(Symbol name, JSVal init, Attributes::Safe attributes) {
    assert(symbol_map_.find(name) == symbol_map_.end());
    symbol_map_[name] = variables_.size();
    variables_.push_back(StoredSlot(init, attributes));
  }

  StoredSlot& PointAt(uint32_t offset) {
    return variables_[offset];
  }

  const Vector& variables() const { return variables_; }

  IV_LV5_INTERNAL_METHOD bool GetOwnNonIndexedPropertySlotMethod(const JSObject* obj, Context* ctx, Symbol name, Slot* slot) {
    const JSGlobal* global = static_cast<const JSGlobal*>(obj);
    const uint32_t entry = global->LookupVariable(name);
    if (entry != core::kNotFound32) {
      const StoredSlot& stored(global->variables_[entry]);
      slot->set(stored.value(), stored.attributes(), obj);
      return true;
    }
    const bool res = JSObject::GetOwnNonIndexedPropertySlotMethod(obj, ctx, name, slot);
    if (!res) {
      slot->MakeUnCacheable();
    }
    return res;
  }

  IV_LV5_INTERNAL_METHOD bool DefineOwnNonIndexedPropertySlotMethod(JSObject* obj,
                                                                    Context* ctx,
                                                                    Symbol name,
                                                                    const PropertyDescriptor& desc,
                                                                    Slot* slot,
                                                                    bool th, Error* e) {
    JSGlobal* global = static_cast<JSGlobal*>(obj);
    const uint32_t entry = global->LookupVariable(name);
    if (entry != core::kNotFound32) {
      StoredSlot stored(global->variables_[entry]);
      bool returned = false;
      if (stored.IsDefineOwnPropertyAccepted(desc, th, &returned, e)) {
        // if desc is accessor, IsDefineOwnPropertyAccepted reject it.
        stored.Merge(ctx, desc);
        global->variables_[entry] = stored;
      }
      return returned;
    }
    return JSObject::DefineOwnNonIndexedPropertySlotMethod(obj, ctx, name, desc, slot, th, e);
  }

  IV_LV5_INTERNAL_METHOD bool DeleteNonIndexedMethod(JSObject* obj, Context* ctx, Symbol name, bool th, Error* e) {
    JSGlobal* global = static_cast<JSGlobal*>(obj);
    const uint32_t entry = global->LookupVariable(name);
    if (entry != core::kNotFound32) {
      // because all variables are configurable:false
      return false;
    }
    return JSObject::DeleteNonIndexedMethod(obj, ctx, name, th, e);
  }

  IV_LV5_INTERNAL_METHOD void GetOwnPropertyNamesMethod(const JSObject* obj,
                                                        Context* ctx,
                                                        PropertyNamesCollector* collector,
                                                        EnumerationMode mode) {
    const JSGlobal* global = static_cast<const JSGlobal*>(obj);
    for (SymbolMap::const_iterator it = global->symbol_map_.begin(),
         last = global->symbol_map_.end(); it != last; ++it) {
      if (mode == INCLUDE_NOT_ENUMERABLE ||
          global->variables_[it->second].attributes().IsEnumerable()) {
        collector->Add(it->first, it->second);
      }
    }
    return JSObject::GetOwnPropertyNamesMethod(obj, ctx, collector->LevelUp(), mode);
  }

  virtual void MarkChildren(radio::Core* core) {
    JSObject::MarkChildren(core);
    for (Vector::const_iterator it = variables_.cbegin(),
         last = variables_.cend(); it != last; ++it) {
      core->MarkValue(it->value());
    }
  }

 private:
  explicit JSGlobal(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx, static_cast<JSObject*>(NULL))),
      symbol_map_(),
      variables_() {
    set_cls(GetClass());
    assert(map()->GetSlotsSize() == 0);
  }

  SymbolMap symbol_map_;
  Vector variables_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSGLOBAL_H_
