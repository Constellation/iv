// placeholder header
#ifndef IV_LV5_SYMBOL_H_
#define IV_LV5_SYMBOL_H_
#include <iv/symbol.h>
#include <iv/lv5/radio/cell.h>
namespace iv {
namespace lv5 {

class Context;

using core::Symbol;

namespace symbol {
using namespace iv::core::symbol;


inline core::Symbol MakePublicSymbol(Context* ctx, radio::Cell* ptr) {
  // T should be GC-managed pointer
  return core::symbol::MakePublicSymbol<radio::Cell>(ptr);
}

template<typename T>
inline core::Symbol MakePrivateSymbol(T* unique_id) {
  // T should not be GC-managed pointer
  return core::symbol::MakePrivateSymbol(reinterpret_cast<uintptr_t>(unique_id));
}

}  // namespace iv::lv5::symbol

} }  // namespace iv::lv5
#endif  // IV_LV5_SYMBOL_H_
