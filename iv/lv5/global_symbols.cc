#include <iv/lv5/global_data.h>
#include <iv/lv5/global_symbols.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jssymbol.h>
#include <iv/lv5/jsstring.h>
namespace iv {
namespace lv5 {

void GlobalSymbols::InitGlobalSymbols(Context* ctx) {
  Error::Dummy dummy;
#define V(name) IV_CONCAT(IV_CONCAT(builtin_symbol_, name), _) =\
  JSSymbol::New(ctx, JSString::NewAsciiString(ctx, "Symbol." #name, &dummy));
  IV_BUILTIN_SYMBOLS(V)
#undef V
}

} }  // namespace iv::lv5
