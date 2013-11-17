#ifndef IV_LV5_JSSYMBOL_H_
#define IV_LV5_JSSYMBOL_H_
#include <iv/ustring.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/private_symbol.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class JSSymbol : public JSCell {
 public:
  static JSSymbol* New(Context* ctx, JSVal description) {
    return new JSSymbol(ctx, description);
  }

  static JSSymbol* Extract(Symbol sym) {
    assert(symbol::IsPrivateSymbol(sym));
    return symbol::GetPtrFromSymbol<JSSymbol>(sym);
  }

  Symbol symbol() const { return symbol_; }
  JSVal description() const { return description_; }

  virtual void MarkChildren(radio::Core* core) {
    core->MarkValue(description_);
    core->MarkCell(Extract(symbol_));
  }
 private:
  JSSymbol(Context* ctx, JSVal description)
    : JSCell(radio::SYMBOL, ctx->global_data()->primitive_symbol_map(), nullptr),
      symbol_(symbol::MakeGCPrivateSymbol(ctx, this)),
      description_(description) { }

  Symbol symbol_;
  JSVal description_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSYMBOL_H_
