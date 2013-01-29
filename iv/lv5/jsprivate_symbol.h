#ifndef IV_LV5_JSPRIVATE_SYMBOL_H_
#define IV_LV5_JSPRIVATE_SYMBOL_H_
#include <iv/ustring.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/private_symbol.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class JSPrivateSymbol : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSPrivateSymbol, PrivateSymbol)

  static JSPrivateSymbol* New(Context* ctx) {
    JSPrivateSymbol* const name =
        new JSPrivateSymbol(ctx, ctx->global_data()->private_symbol_map());
    name->set_cls(GetClass());
    return name;
  }

  static JSPrivateSymbol* NewPlain(Context* ctx, Map* map) {
    return new JSPrivateSymbol(ctx, map);
  }

  Symbol symbol() const { return symbol_; }
 private:
  JSPrivateSymbol(Context* ctx, Map* map)
    : JSObject(map),
      symbol_(symbol::MakeGCPrivateSymbol(ctx)) { }

  Symbol symbol_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSPRIVATE_SYMBOL_H_
