#ifndef _IV_LV5_DEFAULT_SYMBOL_PROVIDER_H_
#define _IV_LV5_DEFAULT_SYMBOL_PROVIDER_H_
#include "detail/cstdint.h"
#include "detail/type_traits.h"
#include "detail/unordered_set.h"
#include "singleton.h"
#include "lv5/symbol_fwd.h"
namespace iv {
namespace lv5 {
namespace symbol {
// default symbols
#define IV_LV5_DEFAULT_SYMBOLS(V)\
    V(length)\
    V(eval)\
    V(arguments)\
    V(caller)\
    V(callee)\
    V(toString)\
    V(valueOf)\
    V(prototype)\
    V(constructor)\
    V(__proto__)

class DefaultSymbolProvider : public core::Singleton<DefaultSymbolProvider> {
 public:
  friend class core::Singleton<DefaultSymbolProvider>;

#define V(sym)\
  Symbol sym() const {\
    return sym##_;\
  }
  IV_LV5_DEFAULT_SYMBOLS(V)
#undef V

  bool IsDefaultSymbol(Symbol sym) const {
    return default_symbols_.find(sym) != default_symbols_.end();
  }

 private:

  DefaultSymbolProvider()
    : default_symbols_() {
#define V(sym)\
  const core::StringPiece sym##_target(#sym);\
  sym##_ = detail::MakeSymbol(new core::UString(sym##_target.begin(), sym##_target.end()));\
  default_symbols_.insert(sym##_);
    IV_LV5_DEFAULT_SYMBOLS(V)
#undef V
  }

  // private destructor
  ~DefaultSymbolProvider() {
    for (std::unordered_set<Symbol>::const_iterator it = default_symbols_.begin(),
         last = default_symbols_.end(); it != last; ++it) {
      assert(IsStringSymbol(*it));
      delete GetStringFromSymbol(*it);
    }
  }

#define V(sym) Symbol sym##_;
  IV_LV5_DEFAULT_SYMBOLS(V)
#undef V
  std::unordered_set<Symbol> default_symbols_;
};

#define V(sym) static const Symbol sym = DefaultSymbolProvider::Instance()->sym();
IV_LV5_DEFAULT_SYMBOLS(V)
#undef V

} } }  // namespace iv::lv5::symbol
#endif  // _IV_LV5_SYMBOL_H_
