#ifndef IV_SYMBOL_H_
#define IV_SYMBOL_H_
#include <cstddef>
#include <iv/symbol_fwd.h>
#include <iv/default_symbol_provider.h>
namespace iv {
namespace core {

typedef const core::UString* StringSymbol;

namespace symbol {

static const Symbol kDummySymbol =
    MakeSymbol(static_cast<core::UString*>(nullptr));

// Symbol key traits for QHashMap
struct KeyTraits {
  static unsigned hash(Symbol val) {
    return std::hash<Symbol>()(val);
  }
  static bool equals(Symbol lhs, Symbol rhs) {
    return lhs == rhs;
  }
  // because of Array index
  static Symbol null() { return kDummySymbol; }
};

} } }  // namespace iv::core::symbol
#endif  // IV_SYMBOL_H_
