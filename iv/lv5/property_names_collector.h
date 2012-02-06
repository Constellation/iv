#ifndef IV_LV5_PROPERTY_NAMES_COLLECTOR_H_
#define IV_LV5_PROPERTY_NAMES_COLLECTOR_H_
#include <vector>
#include <iv/lv5/symbol.h>
namespace iv {
namespace lv5 {

class PropertyNamesCollector {
 public:
  typedef core::SortedVector<SymbolKey> Names;
  class SymbolKey {
   public:
    SymbolKey(Symbol sym, uint32_t order, uint32_t level)
      : symbol_(sym),
        order_((level << 32) | order) {
    }

    friend bool operator<(const SymbolKey& lhs, const SymbolKey& rhs) {
      if (symbol::IsIndexSymbol(lhs.symbol)) {
        if (symbol::IsIndexSymbol(rhs.symbol)) {
          const uint32_t li = symbol::GetIndexFromSymbol(lhs.symbol);
          const uint32_t ri = symbol::GetIndexFromSymbol(rhs.symbol);
          return li < ri;
        }
        return true;
      }
      if (symbol::IsIndexSymbol(rhs.symbol)) {
        return false;
      }
      return lhs.order < rhs.order;
    }

    Symbol symbol() const {
      return symbol_;
    }
   private:
    Symbol symbol_;
    uint64_t order_;
  };

  PropertyNamesCollector()
    : names_(),
      level_(0) {
  }

  const Names& names() const { return names_; }

  void Add(Symbol symbol, uint32_t order) {
    for (Names::const_iterator it = names_.begin(),
         last = names_.end(); it != last; ++it) {
      if (it->symbol() == symbol) {
        return;
      }
    }
    names_.push_back(SymbolKey(symbol, order, level_));
  }

  PropertyNamesCollector* LevelUp() {
    level_ += 1;
    return this;
  }

 private:
  Names names_;
  uint32_t level_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_PROPERTY_NAMES_COLLECTOR_H_
