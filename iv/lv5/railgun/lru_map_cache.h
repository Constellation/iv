#ifndef IV_LV5_RAILGUN_LRU_MAP_CACHE_H_
#define IV_LV5_RAILGUN_LRU_MAP_CACHE_H_
#include <list>
#include <utility>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/map.h>
namespace iv {
namespace lv5 {
namespace railgun {

class LRUMapCache {
 public:
  typedef std::pair<Map*, Symbol> key_type;

  struct Hasher {
    std::size_t operator()(const key_type& key) const {
      return std::hash<Map*>()(key.first) + std::hash<Symbol>()(key.second);
    }
  };

  struct Equaler {
    bool operator()(const key_type& lhs, const key_type& rhs) const {
      return lhs.first == rhs.first && lhs.second == rhs.second;
    }
  };

  typedef std::list<key_type> HistoryList;
  typedef std::pair<std::size_t, HistoryList::iterator> Entry;

  typedef GCHashMap<
      key_type,
      Entry,
      Hasher, Equaler>::type MapCache;

  explicit LRUMapCache(std::size_t max)
    : max_(max),
      list_(),
      map_() {
  }

  std::size_t Lookup(const key_type& key) {
    const MapCache::iterator it = map_.find(key);
    if (it == map_.end()) {
      return core::kNotFound;
    }

    list_.splice(list_.end(), list_, it->second.second);
    return it->second.first;
  }

  void Insert(const key_type& key, std::size_t offset) {
    assert(Lookup(key) == core::kNotFound);

    if (list_.size() == max_) {
      // remove LRU entry
      const MapCache::iterator it = map_.find(list_.front());
      assert(it != map_.end());
      map_.erase(it);
      list_.pop_front();
      assert((list_.size() + 1) == max_);
    }

    const HistoryList::iterator it = list_.insert(list_.end(), key);
    map_.insert(std::make_pair(key, Entry(offset, it)));
  }

 private:
  std::size_t max_;
  HistoryList list_;
  MapCache map_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_LRU_MAP_CACHE_H_
