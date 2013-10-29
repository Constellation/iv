#include <iv/detail/cstdint.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jssymbol.h>
#include <iv/lv5/runtime/symbol.h>
namespace iv {
namespace lv5 {
namespace runtime {

JSVal SymbolConstructor(const Arguments& args, Error* e) {
  return JSSymbol::New(args.ctx());
}

JSVal SymbolIsSymbol(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Symbol.isSymbol", args, e);
  if (args.empty()) {
    return JSFalse;
  }
  const JSVal val = args.front();
  if (!val.IsObject()) {
    return JSFalse;
  }
  return JSVal::Bool(val.object()->IsClass<Class::Symbol>());
}

} } }  // namespace iv::lv5::runtime
