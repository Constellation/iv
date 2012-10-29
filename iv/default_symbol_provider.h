#ifndef IV_DEFAULT_SYMBOL_PROVIDER_H_
#define IV_DEFAULT_SYMBOL_PROVIDER_H_
#include <iv/detail/cstdint.h>
#include <iv/detail/type_traits.h>
#include <iv/detail/unordered_set.h>
#include <iv/singleton.h>
#include <iv/symbol_fwd.h>
namespace iv {
namespace core {
namespace symbol {
// default symbols
#define IV_DEFAULT_SYMBOLS(V)\
    V(length)\
    V(eval)\
    V(arguments)\
    V(caller)\
    V(callee)\
    V(toString)\
    V(toLocaleString)\
    V(toJSON)\
    V(valueOf)\
    V(prototype)\
    V(constructor)\
    V(undefined)\
    V(NaN)\
    V(Infinity)\
    V(null)\
    V(name)\
    V(message)\
    V(stack)\
    V(get)\
    V(set)\
    V(value)\
    V(configurable)\
    V(writable)\
    V(enumerable)\
    V(lastIndex)\
    V(index)\
    V(input)\
    V(ignoreCase)\
    V(multiline)\
    V(global)\
    V(source)\
    V(compare)\
    V(join)\
    V(Intl)\
    V(Collator)\
    V(NumberFormat)\
    V(DateTimeFormat)\
    V(usage)\
    V(localeMatcher)\
    V(style)\
    V(currency)\
    V(currencyDisplay)\
    V(minimumIntegerDigits)\
    V(minimumFractionDigits)\
    V(maximumFractionDigits)\
    V(minimumSignificantDigits)\
    V(maximumSignificantDigits)\
    V(useGrouping)\
    V(timeZone)\
    V(hour12)\
    V(formatMatcher)\
    V(raw)\
    V(byteLength)\
    V(buffer)\
    V(byteOffset)\
    V(add)

class DefaultSymbolProvider : public core::Singleton<DefaultSymbolProvider> {
 public:
  friend class core::Singleton<DefaultSymbolProvider>;

#define V(sym)\
  Symbol sym() const {\
    return sym##_;\
  }
  IV_DEFAULT_SYMBOLS(V)
#undef V

  bool IsDefaultSymbol(Symbol sym) const {
    return default_symbols_.find(sym) != default_symbols_.end();
  }

 private:

  DefaultSymbolProvider()
    : default_symbols_() {
#define V(sym)\
  const core::StringPiece sym##_target(#sym);\
  sym##_ = detail::MakeSymbol(\
      new core::UString(sym##_target.begin(), sym##_target.end()));\
  default_symbols_.insert(sym##_);
    IV_DEFAULT_SYMBOLS(V)
#undef V
  }

  // private destructor
  ~DefaultSymbolProvider() {
    for (std::unordered_set<Symbol>::const_iterator it =
         default_symbols_.begin(), last = default_symbols_.end();
         it != last; ++it) {
      assert(IsStringSymbol(*it));
      delete GetStringFromSymbol(*it);
    }
  }

#define V(sym) Symbol sym##_;
  IV_DEFAULT_SYMBOLS(V)
#undef V
  std::unordered_set<Symbol> default_symbols_;
};

#define V(sym)\
  inline Symbol sym() {\
    static const Symbol target = DefaultSymbolProvider::Instance()->sym();\
    return target;\
  }
IV_DEFAULT_SYMBOLS(V)
#undef V

} } }  // namespace iv::core::symbol
#endif  // IV_SYMBOL_H_
