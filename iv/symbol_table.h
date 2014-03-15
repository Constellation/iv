#ifndef IV_SYMBOLTABLE_H_
#define IV_SYMBOLTABLE_H_
#include <string>
#include <vector>
#include <iv/detail/unordered_map.h>
#include <iv/conversions.h>
#include <iv/symbol.h>
namespace iv {
namespace core {

class SymbolTable {
 public:
  class SymbolHolder {
   public:
    SymbolHolder(const u16string_view& piece)  // NOLINT
      : rep_(),
        size_(piece.size()),
        is_8bit_(false),
        pointer_(nullptr) {
      rep_.rep16 = piece.data();
    }

    SymbolHolder(const string_view& piece)  // NOLINT
      : rep_(),
        size_(piece.size()),
        is_8bit_(true),
        pointer_(nullptr) {
      rep_.rep8 = piece.data();
    }

    SymbolHolder(const std::u16string* str)  // NOLINT
      : rep_(),
        size_(str->size()),
        is_8bit_(false),
        pointer_(str),
        hash_(Hash::StringToHash(*str)) {
      rep_.rep16 = str->data();
    }

    std::size_t hash() const {
      if (pointer_) {
        return hash_;
      }
      if (is_8bit_) {
        return Hash::StringToHash(string_view(rep_.rep8, size_));
      } else {
        return Hash::StringToHash(u16string_view(rep_.rep16, size_));
      }
    }

    friend bool operator==(const SymbolHolder& lhs, const SymbolHolder& rhs) {
      if (lhs.is_8bit_ == rhs.is_8bit_) {
        if (lhs.is_8bit_) {
          // 8bits
          return
              string_view(lhs.rep_.rep8, lhs.size_) ==
              string_view(rhs.rep_.rep8, rhs.size_);
        }
        return
            u16string_view(lhs.rep_.rep16, lhs.size_) ==
            u16string_view(rhs.rep_.rep16, rhs.size_);
      } else {
        if (lhs.size_ != rhs.size_) {
          return false;
        }
        if (lhs.is_8bit_) {
          return CompareIterators(
              reinterpret_cast<const uint8_t*>(lhs.rep_.rep8),
              reinterpret_cast<const uint8_t*>(lhs.rep_.rep8) + lhs.size_,
              rhs.rep_.rep16,
              rhs.rep_.rep16 + rhs.size_
              ) == 0;
        } else {
          return CompareIterators(
              lhs.rep_.rep16,
              lhs.rep_.rep16 + lhs.size_,
              reinterpret_cast<const uint8_t*>(rhs.rep_.rep8),
              reinterpret_cast<const uint8_t*>(rhs.rep_.rep8) + rhs.size_
              ) == 0;
        }
      }
    }

    const std::u16string* pointer() const { return pointer_; }

    std::size_t size() const { return size_; }

   private:
    union Representation {
      const char* rep8;
      const char16_t* rep16;
    } rep_;
    std::size_t size_;
    bool is_8bit_;
    const std::u16string* pointer_;
    std::size_t hash_;
  };

  struct SymbolHolderHasher {
    inline std::size_t operator()(const SymbolHolder& x) const {
      return x.hash();
    }
  };

  typedef std::unordered_set<SymbolHolder, SymbolHolderHasher> Set;

  explicit SymbolTable(bool insert_default_symbol_flag = true)
    : set_(),
      insert_default_symbol_(insert_default_symbol_flag) {
    // insert default symbols
    if (insert_default_symbol()) {
#define V(sym) InsertDefaults(symbol::sym());
      IV_DEFAULT_SYMBOLS(V)
#undef V
    }
  }

  ~SymbolTable() {
    for (Set::const_iterator it = set_.begin(),
         last = set_.end(); it != last; ++it) {
      if (!insert_default_symbol() ||
          !symbol::DefaultSymbolProvider::Instance()->IsDefaultSymbol(
              symbol::MakeSymbol(it->pointer()))) {
        delete it->pointer();
      }
    }
  }

  template<class CharT>
  inline Symbol Lookup(const CharT* str) {
    return Lookup(basic_string_view<CharT>(str));
  }

  template<class String>
  inline Symbol Lookup(const String& str) {
    uint32_t index;
    if (ConvertToUInt32(str.begin(), str.end(), &index)) {
      return symbol::MakeSymbolFromIndex(index);
    }
    const SymbolHolder target(str);
    typename Set::const_iterator it = set_.find(target);
    if (it != set_.end()) {
      return symbol::MakeSymbol(it->pointer());
    } else {
      const std::u16string* res = new std::u16string(str.begin(), str.end());
      set_.insert(res);
      return symbol::MakeSymbol(res);
    }
  }

  bool insert_default_symbol() const { return insert_default_symbol_; }

 private:
  void InsertDefaults(Symbol sym) {
    if (symbol::IsStringSymbol(sym)) {
      assert(set_.find(symbol::GetStringFromSymbol(sym)) == set_.end());
      set_.insert(symbol::GetStringFromSymbol(sym));
    }
  }

  Set set_;
  bool insert_default_symbol_;
};

} }  // namespace iv::core
#endif  // IV_SYMBOLTABLE_H_
