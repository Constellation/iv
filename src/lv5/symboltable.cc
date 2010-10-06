#include "symboltable.h"
namespace iv {
namespace lv5 {

SymbolTable::SymbolTable()
  : sync_(),
    table_(),
    strings_() {
}

JSString* SymbolTable::ToString(Context* ctx, Symbol sym) const {
  const u16string& str = strings_[sym];
  return JSString::New(ctx, str);
}

} }  // namespace iv::lv5
