#ifndef IV_LV5_JSSYMBOLOBJECT_H_
#define IV_LV5_JSSYMBOLOBJECT_H_
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jssymbol.h>
#include <iv/lv5/map.h>
namespace iv {
namespace lv5 {

class JSSymbolObject : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSSymbolObject, Symbol)

  JSSymbol* symbol() {
    return symbol_;
  }

  static JSSymbolObject* New(Context* ctx, JSSymbol* symbol) {
    JSSymbolObject* const obj = new JSSymbolObject(ctx, symbol);
    obj->set_cls(JSSymbolObject::GetClass());
    return obj;
  }

  static JSSymbolObject* NewPlain(Context* ctx, Map* map, JSSymbol* symbol) {
    return new JSSymbolObject(ctx, map, symbol);
  }

  virtual void MarkChildren(radio::Core* core) {
    // TODO(Yusuke Suzuki): Mark symbol value
    core->MarkCell(symbol_);
  }
 private:
  JSSymbolObject(Context* ctx, JSSymbol* symbol)
    : JSObject(ctx->global_data()->symbol_map()),
      symbol_(symbol) {
  }

  JSSymbolObject(Context* ctx, Map* map, JSSymbol* symbol)
    : JSObject(map),
      symbol_(symbol) {
  }

  JSSymbol* symbol_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSYMBOLOBJECT_H_
