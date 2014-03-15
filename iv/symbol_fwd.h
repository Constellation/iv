#ifndef IV_SYMBOL_FWD_H_
#define IV_SYMBOL_FWD_H_
#include <functional>
#include <string>
#include <iv/detail/cstdint.h>
#include <iv/detail/cinttypes.h>
#include <iv/detail/functional.h>
#include <iv/platform.h>
#include <iv/byteorder.h>
#include <iv/unicode.h>
#include <iv/stringpiece.h>
#include <iv/conversions.h>
namespace iv {
namespace core {
namespace symbol {

template<std::size_t PointerSize, bool IsLittle>
struct SymbolLayout;

template<>
struct SymbolLayout<4, true> {
  typedef SymbolLayout<4, true> this_type;
  union {
    struct {
      uint32_t high_;
      uint32_t low_;
    } index_;
    struct {
      uintptr_t str_;
      uint32_t low_;
    } str_;
    uint64_t bytes_;
  };
};

template<>
struct SymbolLayout<8, true> {
  typedef SymbolLayout<8, true> this_type;
  union {
    struct {
      uint32_t high_;
      uint32_t low_;
    } index_;
    struct {
      uintptr_t str_;
    } str_;
    uint64_t bytes_;
  };
};

template<>
struct SymbolLayout<4, false> {
  typedef SymbolLayout<4, false> this_type;
  union {
    struct {
      uint32_t low_;
      uint32_t high_;
    } index_;
    struct {
      uint32_t low_;
      uintptr_t str_;
    } str_;
    uint64_t bytes_;
  };
};

template<>
struct SymbolLayout<8, false> {
  typedef SymbolLayout<8, false> this_type;
  union {
    struct {
      uint32_t low_;
      uint32_t high_;
    } index_;
    struct {
      uintptr_t str_;
    } str_;
    uint64_t bytes_;
  };
};

typedef SymbolLayout<core::Size::kPointerSize, core::kLittleEndian> Symbol;

static const uint32_t kSymbolIsIndex = 0xFFFF;

inline Symbol MakeSymbol(const std::u16string* str) {
  Symbol symbol = { };
  symbol.str_.str_ = (reinterpret_cast<uintptr_t>(str) | 0x1);
  return symbol;
}

inline Symbol MakeSymbol(uint32_t index) {
  Symbol symbol;
  symbol.index_.high_ = index;
  symbol.index_.low_ = kSymbolIsIndex;
  return symbol;
}

template<typename T>
inline Symbol MakePublicSymbol(T* str) {
  Symbol symbol = { };
  symbol.str_.str_ = reinterpret_cast<uintptr_t>(str);
  return symbol;
}

inline Symbol MakePrivateSymbol(uintptr_t str) {
  Symbol symbol = { };
  symbol.str_.str_ = (str | 0x2);
  return symbol;
}

inline bool operator==(Symbol x, Symbol y) {
  return x.bytes_ == y.bytes_;
}

inline bool operator!=(Symbol x, Symbol y) {
  return x.bytes_ != y.bytes_;
}

inline bool operator<(Symbol x, Symbol y) {
  return x.bytes_ < y.bytes_;
}

inline bool operator>(Symbol x, Symbol y) {
  return x.bytes_ > y.bytes_;
}

inline bool operator<=(Symbol x, Symbol y) {
  return x.bytes_ <= y.bytes_;
}

inline bool operator>=(Symbol x, Symbol y) {
  return x.bytes_ >= y.bytes_;
}

static_assert(std::is_pod<Symbol>::value, "Symbol should be POD");

static const uint32_t kMaxSize = INT32_MAX;

inline bool IsIndexSymbol(Symbol sym) {
  return sym.index_.low_ == kSymbolIsIndex;
}

inline bool IsArrayIndexSymbol(Symbol sym) {
  return
      (sym.index_.low_ == kSymbolIsIndex) &&
      (sym.index_.high_ < UINT32_MAX);
}

inline bool IsStringSymbol(Symbol sym) {
  return !IsIndexSymbol(sym) && (sym.str_.str_ & 0x1);
}

inline bool IsPrivateSymbol(Symbol sym) {
  return !IsIndexSymbol(sym) && (sym.str_.str_ & 0x2);
}

inline bool IsPublicSymbol(Symbol sym) {
  return !IsIndexSymbol(sym) && !(sym.str_.str_ & 0x3);
}

inline Symbol MakeSymbolFromIndex(uint32_t index) {
  return MakeSymbol(index);
}

inline const std::u16string* GetStringFromSymbol(Symbol sym) {
  assert(IsStringSymbol(sym));
  return reinterpret_cast<const std::u16string*>(
      sym.str_.str_ & ~static_cast<uintptr_t>(1));
}

inline uint32_t GetIndexFromSymbol(Symbol sym) {
  assert(IsIndexSymbol(sym));
  return sym.index_.high_;
}

inline std::u16string GetIndexStringFromSymbol(Symbol sym) {
  assert(IsIndexSymbol(sym));
  const uint32_t index = GetIndexFromSymbol(sym);
  std::array<char, 15> buffer;
  char* end = core::UInt32ToString(index, buffer.data());
  return std::u16string(buffer.data(), end);
}

inline std::u16string GetSymbolString(Symbol sym) {
  if (IsIndexSymbol(sym)) {
    return GetIndexStringFromSymbol(sym);
  } else if (IsStringSymbol(sym)) {
    return *GetStringFromSymbol(sym);
  } else {
    return std::u16string();
  }
}

template<typename T>
inline T* GetPtrFromSymbol(Symbol sym) {
  assert(IsPrivateSymbol(sym));
  return reinterpret_cast<T*>(sym.str_.str_);
}

inline std::ostream& operator<<(std::ostream& o, Symbol symbol) {
  if (symbol::IsIndexSymbol(symbol)) {
    return o << symbol::GetIndexFromSymbol(symbol);
  } else {
    const std::u16string* str = symbol::GetStringFromSymbol(symbol);
    if (str) {
      std::string utf8;
      utf8.reserve(str->size());
      if (unicode::UTF16ToUTF8(
              str->begin(), str->end(),
              std::back_inserter(utf8)) == unicode::UNICODE_NO_ERROR) {
        return o << utf8;
      }
    }
  }
  return o;
}

}  // namespace iv::core::symbol

typedef symbol::Symbol Symbol;

} }  // namespace iv::core
namespace IV_HASH_NAMESPACE_START {

// template specialization for Symbol in std::unordered_map
template<>
struct hash<iv::core::Symbol>
  : public std::unary_function<iv::core::Symbol, std::size_t> {
  inline result_type operator()(const argument_type& x) const {
    return hash<uint64_t>()(x.bytes_);
  }
};

} IV_HASH_NAMESPACE_END  // namespace std
#endif  // IV_SYMBOL_FWD_H_
