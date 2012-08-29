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
  delete reinterpret_cast<T*>(obj);
}

inline core::Symbol MakeGCPrivateSymbol(Context* ctx) {
  // create empty unique string
  const core::UString* string =
      new (GC, Delete<core::UString>) core::UString();
  return core::detail::MakePrivateSymbol(string);
}

inline core::Symbol MakePrivateSymbol(Context* ctx, const core::UString* ptr) {
  // create empty unique string
  return core::detail::MakePrivateSymbol(ptr);
}

} } }  // namespace iv::lv5::symbol
#endif  // IV_LV5_PRIVATE_SYMBOL_H_
