#ifndef IV_LV5_RUNTIME_SYMBOL_H_
#define IV_LV5_RUNTIME_SYMBOL_H_
#include <iv/detail/cstdint.h>
#include <iv/conversions.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jssymbol.h>
namespace iv {
namespace lv5 {
namespace runtime {

inline JSVal SymbolConstructor(const Arguments& args, Error* e) {
  return JSSymbol::New(args.ctx());
}

inline JSVal SymbolIsSymbol(const Arguments& args, Error* e) {
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
#endif  // IV_LV5_RUNTIME_SYMBOL_H_
