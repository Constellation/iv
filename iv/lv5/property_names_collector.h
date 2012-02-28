#ifndef IV_LV5_PROPERTY_NAMES_COLLECTOR_H_
#define IV_LV5_PROPERTY_NAMES_COLLECTOR_H_
#include <vector>
#include <iv/lv5/symbol.h>
#include <iv/sorted_vector.h>
namespace iv {
namespace lv5 {

class PropertyNamesCollector {
 public:
  static const std::size_t kReservedSize = 20;
  class SymbolKey {
   public:
    SymbolKey(Symbol sym, uint32_t order, uint32_t level)
      : symbol_(sym),
        order_((static_cast<uint64_t>(level) << 32) | order) {
    }

    friend bool operator<(const SymbolKey& lhs, const SymbolKey& rhs) {
      if (symbol::IsIndexSymbol(lhs.symbol_)) {
        if (symbol::IsIndexSymbol(rhs.symbol_)) {
          const uint32_t li = symbol::GetIndexFromSymbol(lhs.symbol_);
          const uint32_t ri = symbol::GetIndexFromSymbol(rhs.symbol_);
          return li < ri;
        }
        return true;
      }
      if (symbol::IsIndexSymbol(rhs.symbol_)) {
        return false;
      }
      return lhs.order_ < rhs.order_;
    }

    operator Symbol() const {
      return symbol_;
    }
   private:
    Symbol symbol_;
    uint64_t order_;
  };

  typedef core::SortedVector<SymbolKey> Names;

  PropertyNamesCollector()
    : names_(),
      level_(0) {
    names_.reserve(kReservedSize);
  }

  const Names& names() const { return names_; }

  void Add(Symbol symbol, uint32_t order) {
    for (Names::const_iterator it = names_.begin(),
         last = names_.end(); it != last; ++it) {
      if (*it == symbol) {
        return;
      }
    }
    names_.push_back(SymbolKey(symbol, order, level_));
  }

  void Add(uint32_t index) {
    Add(symbol::MakeSymbolFromIndex(index), 0);
  }

  PropertyNamesCollector* LevelUp() {
    level_ += 1;
    return this;
  }

  void Clear() {
    level_ = 0;
    names_.clear();
  }

 private:
  Names names_;
  uint32_t level_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_PROPERTY_NAMES_COLLECTOR_H_
