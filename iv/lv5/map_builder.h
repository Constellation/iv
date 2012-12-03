#ifndef IV_LV5_MAP_BUILDER_H_
#define IV_LV5_MAP_BUILDER_H_
#include <iv/lv5/map.h>
namespace iv {
namespace lv5 {

class MapBuilder {
 public:
  explicit MapBuilder(Context* ctx, JSObject* prototype)
    : ctx_(ctx),
      table_(new(GC)Map::TargetTable()),
      prototype_(prototype) { }

  Map* Build(bool unique = false, bool indexed = false) { return new Map(table_, prototype_, unique, indexed); }

  void Add(Symbol symbol, std::size_t index, Attributes::Safe attributes) {
    assert(Find(symbol).IsNotFound());
    table_->insert(std::make_pair(symbol, Map::Entry(index, attributes)));
  }

  Map::Entry Add(Symbol symbol, Attributes::Safe attributes) {
    assert(Find(symbol).IsNotFound());
    const std::size_t index = table_->size();
    const Map::Entry entry(index, attributes);
    table_->insert(std::make_pair(symbol, entry));
    return entry;
  }

  void Override(Symbol symbol, Map::Entry entry) {
    (*table_)[symbol] = entry;
  }

  Map::Entry Find(Symbol symbol) const {
    const Map::TargetTable::const_iterator it = table_->find(symbol);
    if (it != table_->end()) {
      return it->second;
    } else {
      return Map::Entry::NotFound();
    }
  }
 private:
  Context* ctx_;
  Map::TargetTable* table_;
  JSObject* prototype_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_MAP_BUILDER_H_
