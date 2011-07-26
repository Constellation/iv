#ifndef _IV_LV5_SYMBOL_FWD_H_
#define _IV_LV5_SYMBOL_FWD_H_
#include <gc/gc_allocator.h>
#include <functional>
#include "detail/cstdint.h"
#include "detail/functional.h"
#include "ustring.h"
#include "ustringpiece.h"
#include "stringpiece.h"
#include "byteorder.h"
#include "platform.h"
#include "static_assert.h"
namespace iv {
namespace lv5 {
namespace detail {

template<std::size_t PointerSize, bool IsLittle>
struct SymbolLayout;

static const uint32_t kSymbolIsIndex = 0xFFFF;

template<>
struct SymbolLayout<4, true> {
  typedef SymbolLayout<4, true> this_type;
  union {
    struct {
      uint32_t high_;
      uint32_t low_;
    } index_;
    struct {
      const core::UString* str_;
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
      const core::UString* str_;
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
      const core::UString* str_;
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
      uint32_t low_;
      const core::UString* str_;
    } str_;
    uint64_t bytes_;
  };
};

typedef detail::SymbolLayout<
    core::Size::kPointerSize,
    core::kLittleEndian> Symbol;

inline Symbol MakeSymbol(const core::UString* str) {
  Symbol symbol = { };
  symbol.str_.str_ = str;
  return symbol;
}

inline Symbol MakeSymbol(uint32_t index) {
  Symbol symbol;
  symbol.index_.high_ = index;
  symbol.index_.low_ = kSymbolIsIndex;
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

}  // namespace detail

typedef detail::Symbol Symbol;

#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 3)
IV_STATIC_ASSERT(std::is_pod<Symbol>::value);
#endif

struct SymbolStringHolder {
  const core::UString* symbolized_;
};

inline bool operator==(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) == (*y.symbolized_);
}

inline bool operator!=(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) != (*y.symbolized_);
}

inline bool operator<(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) < (*y.symbolized_);
}

inline bool operator>(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) > (*y.symbolized_);
}

inline bool operator<=(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) <= (*y.symbolized_);
}

inline bool operator>=(SymbolStringHolder x, SymbolStringHolder y) {
  return (*x.symbolized_) >= (*y.symbolized_);
}

namespace symbol {

inline bool IsIndexSymbol(Symbol sym) {
  return sym.index_.low_ & 0x1;
}

inline bool IsStringSymbol(Symbol sym) {
  return !IsIndexSymbol(sym);
}

inline Symbol MakeSymbolFromIndex(uint32_t index) {
  return detail::MakeSymbol(index);
}

inline const core::UString* GetStringFromSymbol(Symbol sym) {
  assert(IsStringSymbol(sym));
  return sym.str_.str_;
}

inline uint32_t GetIndexFromSymbol(Symbol sym) {
  assert(IsIndexSymbol(sym));
  return sym.index_.high_;
}

inline core::UString GetIndexStringFromSymbol(Symbol sym) {
  assert(IsIndexSymbol(sym));
  const uint32_t index = GetIndexFromSymbol(sym);
  std::array<char, 15> buf;
  return core::UString(
      buf.data(),
      buf.data() + snprintf(
          buf.data(), buf.size(), "%lu",
          static_cast<unsigned long>(index)));  // NOLINT
}

inline core::UString GetSymbolString(Symbol sym) {
  if (IsIndexSymbol(sym)) {
    return GetIndexStringFromSymbol(sym);
  } else {
    return *GetStringFromSymbol(sym);
  }
}

} } }  // namespace iv::lv5::symbol

GC_DECLARE_PTRFREE(iv::lv5::Symbol);

namespace IV_HASH_NAMESPACE_START {

// template specialization for Symbol in std::unordered_map
template<>
struct hash<iv::lv5::Symbol>
  : public std::unary_function<iv::lv5::Symbol, std::size_t> {
  inline result_type operator()(const argument_type& x) const {
    return hash<uint64_t>()(x.bytes_);
  }
};

// template specialization for SymbolStringHolder in std::unordered_map
template<>
struct hash<iv::lv5::SymbolStringHolder>
  : public std::unary_function<iv::lv5::SymbolStringHolder, std::size_t> {
  inline result_type operator()(const argument_type& x) const {
    return hash<iv::core::UString>()(*x.symbolized_);
  }
};

} IV_HASH_NAMESPACE_END
#endif  // _IV_LV5_SYMBOL_FWD_H_
