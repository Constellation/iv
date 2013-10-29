#ifndef IV_LV5_RUNTIME_SYMBOL_H_
#define IV_LV5_RUNTIME_SYMBOL_H_
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
namespace iv {
namespace lv5 {
namespace runtime {

JSVal SymbolConstructor(const Arguments& args, Error* e);

JSVal SymbolIsSymbol(const Arguments& args, Error* e);

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_SYMBOL_H_
