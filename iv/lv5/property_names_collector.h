// PropertyNamesCollector ensures property names enumeration order
// Order is defined in
//   http://wiki.ecmascript.org/doku.php?id=strawman:enumeration
//
// 1. index properties (see definition below) in ascending numeric order
// 2. all other properties, in the order in which they were created
//
#ifndef IV_LV5_PROPERTY_NAMES_COLLECTOR_H_
#define IV_LV5_PROPERTY_NAMES_COLLECTOR_H_
#include <vector>
#include <iv/lv5/symbol.h>
#include <iv/sorted_vector.h>
#include <iv/lv5/enumeration_mode.h>
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

    explicit SymbolKey(uint32_t index)
      : symbol_(symbol::MakeSymbolFromIndex(index)),
        order_(0) {
    }

    operator Symbol() const {
      return symbol_;
    }

    uint32_t index() const {
      assert(symbol::IsIndexSymbol(symbol_));
      return symbol::GetIndexFromSymbol(symbol_);
    }

    uint64_t order() const {
      return order_;
    }
   private:
    Symbol symbol_;
    uint64_t order_;
  };

  struct IndexCompare {
    bool operator()(const SymbolKey& lhs, const SymbolKey& rhs) const {
      return lhs.index() < rhs.index();
    }
  };

  struct SymbolCompare {
    bool operator()(const SymbolKey& lhs, const SymbolKey& rhs) const {
      return lhs.order() < rhs.order();
    }
  };

  typedef trace::Vector<SymbolKey>::type Names;

  PropertyNamesCollector()
    : names_(),
      level_(0),
      index_position_(0) {
    names_.reserve(kReservedSize);
  }

  const Names& names() const { return names_; }

  void Add(Symbol symbol, uint32_t order) {
    if (symbol::IsIndexSymbol(symbol)) {
      Add(symbol::GetIndexFromSymbol(symbol));
    } else {
      const SymbolKey key(symbol, order, level_);
      const Names::iterator last = names_.end();
      const Names::iterator it =
          std::lower_bound(names_.begin() + index_position_,
                           last, key, SymbolCompare());
      if (it == last || *it != symbol) {
        names_.insert(it, key);
      }
    }
  }

  void Add(uint32_t index) {
    const SymbolKey key(index);
    const Names::iterator last = names_.begin() + index_position_;
    const Names::iterator it =
        std::lower_bound(names_.begin(), last, key, IndexCompare());
    if (it == last || it->index() != index) {
      // insert new entry
      names_.insert(it, key);
      index_position_ += 1;
    }
  }

  PropertyNamesCollector* LevelUp() {
    level_ += 1;
    return this;
  }

  void Clear() {
    names_.clear();
    level_ = 0;
    index_position_ = 0;
  }

 private:
  Names names_;
  uint32_t level_;
  uint32_t index_position_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_PROPERTY_NAMES_COLLECTOR_H_
