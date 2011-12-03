#ifndef IV_SYMBOLTABLE_H_
#define IV_SYMBOLTABLE_H_
#include <string>
#include <vector>
#include <iv/detail/unordered_map.h>
#include <iv/ustring.h>
#include <iv/conversions.h>
#include <iv/symbol.h>
namespace iv {
namespace core {

class SymbolTable {
 public:
  typedef std::unordered_set<
      const iv::core::UString*,
      SymbolHolderHasher, SymbolHolderEqualTo> Set;

  SymbolTable()
    : set_() {
    // insert default symbols
#define V(sym) InsertDefaults(symbol::sym());
    IV_DEFAULT_SYMBOLS(V)
#undef V
  }

  ~SymbolTable() {
    for (Set::const_iterator it = set_.begin(),
         last = set_.end(); it != last; ++it) {
      const Symbol sym = detail::MakeSymbol(*it);
      if (!symbol::DefaultSymbolProvider::Instance()->IsDefaultSymbol(sym)) {
        delete *it;
      }
    }
  }

  template<class CharT>
  inline Symbol Lookup(const CharT* str) {
    using std::char_traits;
    return Lookup(core::BasicStringPiece<CharT>(str));
  }

  template<class String>
  inline Symbol Lookup(const String& str) {
    uint32_t index;
    if (core::ConvertToUInt32(str.begin(), str.end(), &index)) {
      return symbol::MakeSymbolFromIndex(index);
    }
    const core::UString* target(new core::UString(str.begin(), str.end()));
    typename Set::const_iterator it = set_.find(target);
    if (it != set_.end()) {
      delete target;
      return detail::MakeSymbol(*it);
    } else {
      set_.insert(target);
      return detail::MakeSymbol(target);
    }
  }

 private:
  void InsertDefaults(Symbol sym) {
    if (symbol::IsStringSymbol(sym)) {
      assert(set_.find(symbol::GetStringFromSymbol(sym)) == set_.end());
      set_.insert(symbol::GetStringFromSymbol(sym));
    }
  }

  Set set_;
};

} }  // namespace iv::core
#endif  // IV_SYMBOLTABLE_H_
