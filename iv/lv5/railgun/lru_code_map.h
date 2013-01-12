#ifndef IV_LV5_RAILGUN_LRU_CODE_MAP_H_
#define IV_LV5_RAILGUN_LRU_CODE_MAP_H_
#include <list>
#include <utility>
#include <iv/qhashmap.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/jsstring_fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

class LRUCodeMap {
 public:
  typedef std::list<JSString*> HistoryList;
  typedef std::pair<Code*, HistoryList::iterator> Entry;

  // JSString key traits for QHashMap
  struct KeyTraits {
    static unsigned hash(JSString* val) {
      return JSString::Hasher()(val);
    }
    static bool equals(JSString* lhs, JSString* rhs) {
      return *lhs == *rhs;
    }
    // because of Array index
    static JSString* null() { return NULL; }
  };

  typedef core::QHashMap<JSString*, Entry, KeyTraits, GCAlloc> CodeMap;

  explicit LRUCodeMap(std::size_t max)
    : max_(max),
      list_(),
      map_() {
  }

  Code* Lookup(JSString* str) {
    const CodeMap::iterator it = map_.find(str);
    if (it == map_.end()) {
      return NULL;
    }

    list_.splice(list_.end(), list_, it->second.second);
    return it->second.first;
  }

  void Insert(JSString* str, Code* code) {
    assert(!Lookup(str));

    if (list_.size() == max_) {
      // remove LRU entry
      const CodeMap::iterator it = map_.find(list_.front());
      assert(it != map_.end());
      map_.erase(it);
      list_.pop_front();
      assert((list_.size() + 1) == max_);
    }

    const HistoryList::iterator it = list_.insert(list_.end(), str);
    map_.insert(std::make_pair(str, Entry(code, it)));
  }

 private:
  std::size_t max_;
  HistoryList list_;
  CodeMap map_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_LRU_CODE_MAP_H_
