// placeholder header
#ifndef IV_LV5_SYMBOL_H_
#define IV_LV5_SYMBOL_H_
#include <iv/symbol.h>
#include <gc/gc_allocator.h>
namespace iv {
namespace lv5 {

using core::Symbol;

namespace symbol {
using namespace iv::core::symbol;
}  // namespace iv::lv5::symbol

} }  // namespace iv::lv5
GC_DECLARE_PTRFREE(iv::lv5::Symbol);
#endif  // IV_LV5_SYMBOL_H_
