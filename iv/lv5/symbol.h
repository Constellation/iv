// placeholder header
#ifndef IV_LV5_SYMBOL_H_
#define IV_LV5_SYMBOL_H_
#include <iv/symbol.h>
#include <iv/lv5/private_symbol.h>
namespace iv {
namespace lv5 {

using core::Symbol;

namespace symbol {
using namespace iv::core::symbol;
}  // namespace iv::lv5::symbol

} }  // namespace iv::lv5

// Because of PrivateName implementation, we comment out pointer free
// declaration of table.
// GC_DECLARE_PTRFREE(iv::lv5::Symbol);

#endif  // IV_LV5_SYMBOL_H_
