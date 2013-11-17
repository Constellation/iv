#ifndef IV_LV5_PRIVATE_SYMBOL_H_
#define IV_LV5_PRIVATE_SYMBOL_H_
#include <gc/gc_cpp.h>
#include <iv/symbol.h>
namespace iv {
namespace lv5 {

class Context;

namespace symbol {

template<typename T>
inline void Delete(void* obj, void* data) {
  reinterpret_cast<T*>(obj)->~T();
}

template<typename T>
inline core::Symbol MakeGCPrivateSymbol(Context* ctx, T* ptr) {
  // T should be GC-controlled pointer
  return core::detail::MakePrivateSymbol<T>(ptr);
}

} } }  // namespace iv::lv5::symbol
#endif  // IV_LV5_PRIVATE_SYMBOL_H_
