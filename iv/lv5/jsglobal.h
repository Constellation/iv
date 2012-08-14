#ifndef IV_LV5_JSGLOBAL_H_
#define IV_LV5_JSGLOBAL_H_
#include <iv/notfound.h>
#include <iv/segmented_vector.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/map.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/class.h>
#include <iv/lv5/arguments.h>
namespace iv {
namespace lv5 {

class JSGlobal : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(global)

  typedef GCHashMap<Symbol, uint32_t>::type SymbolMap;
  typedef core::SegmentedVector<JSVal, 8, gc_allocator<JSVal> > Vector;

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

  inline JSVal* LookupVariable(Symbol name) {
    const SymbolMap::const_iterator it = symbol_map_.find(name);
    if (it != symbol_map_.end()) {
      return &variables_[it->second];
    }
    return NULL;
  }

  JSVal* PushVariable(Symbol name, JSVal init = JSUndefined) {
    assert(symbol_map_.find(name) == symbol_map_.end());
    symbol_map_[name] = variables_.size();
    variables_.push_back(init);
    return &variables_.back();
  }

  const Vector& variables() const { return variables_; }

  virtual bool GetOwnPropertySlot(Context* ctx, Symbol name, Slot* slot) const {
    return JSObject::GetOwnPropertySlot(ctx, name, slot);
  }

  virtual bool DefineOwnProperty(Context* ctx,
                                 Symbol name, const PropertyDescriptor& desc,
                                 bool th, Error* e) {
    return JSObject::DefineOwnProperty(ctx, name, desc, th, e);
  }

  virtual bool Delete(Context* ctx, Symbol name, bool th, Error* e) {
    return JSObject::Delete(ctx, name, th, e);
  }

  virtual void GetOwnPropertyNames(Context* ctx,
                                   PropertyNamesCollector* collector,
                                   EnumerationMode mode) const {
    return GetOwnPropertyNames(ctx, collector, mode);
  }

  virtual void MarkChildren(radio::Core* core) {
    JSObject::MarkChildren(core);
    std::for_each(variables_.begin(), variables_.end(), radio::Core::Marker(core));
  }

 private:
  explicit JSGlobal(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      symbol_map_(),
      variables_() {
    assert(map()->GetSlotsSize() == 0);
  }

  SymbolMap symbol_map_;
  Vector variables_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSGLOBAL_H_
