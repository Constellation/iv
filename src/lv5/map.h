// Map object is class like object
// This classes are used for implementing Polymorphic Inline Cache.
//
// original paper is
//   http://cs.au.dk/~hosc/local/LaSC-4-3-pp243-281.pdf
//
#ifndef _IV_LV5_MAP_H_
#define _IV_LV5_MAP_H_
#include <gc/gc_cpp.h>
#include <string>
#include <limits>
#include "lv5/jsobject.h"
#include "lv5/symbol.h"
#include "lv5/property.h"
#include "lv5/gc_template.h"
#include "lv5/context_utils.h"
namespace iv {
namespace lv5 {

// TODO(Constellation)
// more clear deleted

class JSGlobal;

static const std::size_t kNotFound = std::numeric_limits<std::size_t>::max();

class Map : public gc {
 public:
  enum Kind {
    GLOBAL = 0
  };
  typedef GCHashMap<Symbol, std::size_t>::type TargetTable;
  typedef GCVector<std::size_t>::type Deleted;
  typedef GCHashMap<Symbol, Map*>::type Transitions;

  explicit Map(Map* previous)
    : previous_(previous),
      table_(NULL),
      transitions_(),
      deleted_(NULL),
      added_() {
  }

  // empty start table
  Map(Kind kind)
    : previous_(NULL),
      table_(new (GC) TargetTable()),
      transitions_(),
      deleted_(NULL),
      added_(),
      unique_(kind == GLOBAL) {
  }

  std::size_t Get(Context* ctx, Symbol name) {
    if (!HasTable()) {
      AllocateTable(this);
    }
    const TargetTable::const_iterator it = table_->find(name);
    if (it != table_->end()) {
      return it->second;
    } else {
      return kNotFound;
    }
  }

  Map* DeletePropertyTransition(Context* ctx, Symbol name) {
    assert(GetSlotsSize() > 0);
    Map* map = new Map(this);
    map->unique_ = unique_;
    map->AllocateTable(this);
    map->Delete(ctx, name);
    if (deleted_ && !deleted_->empty()) {
      map->deleted_->insert(
          map->deleted_->end(),
          deleted_->begin(),
          deleted_->end());
    }
    return map;
  }

  void Delete(Context* ctx, Symbol name) {
    assert(HasTable());
    const TargetTable::const_iterator it = table_->find(name);
    assert(it != table_->end());
    if (!deleted_) {
      deleted_ = new (GC) Deleted();
    }
    deleted_->push_back(it->second);
    table_->erase(it);
  }

  Map* AddPropertyTransition(Context* ctx, Symbol name, std::size_t* offset) {
    if (unique_) {
      // extend this map
      assert(table_);
      std::size_t slot;
      if (deleted_) {
        slot = deleted_->back();
        if (deleted_->size() == 1) {
          deleted_ = NULL;
        }
      } else {
        slot = GetSlotsSize();
      }
      table_->insert(std::make_pair(name, slot));
      *offset = slot;
      return this;
    } else {
      const Transitions::const_iterator it = transitions_.find(name);
      if (it != transitions_.end()) {
        // found already created map. so, move to this
        return it->second;
      }
      Map* map = new Map(this);
      if (deleted_) {
        assert(!deleted_->empty());
        // use deleted slot as new slot
        const std::size_t slot = deleted_->back();
        map->added_ = std::make_pair(name, slot);
        if (deleted_->size() != 1) {
          // more than 1, so copy this and add to newly created map
          map->deleted_ = new (GC) Deleted();
          map->deleted_->insert(map->deleted_->end(),
                                deleted_->begin() + 1,
                                deleted_->end());
        }
      } else {
        map->added_ = std::make_pair(name, GetSlotsSize());
      }
      transitions_.insert(std::make_pair(name, map));
      *offset = map->added_.second;
      return map;
    }
  }

  std::size_t GetSlotsSize() const {
    if (table_) {
      return table_->size() + ((deleted_) ? deleted_->size() : 0);
    } else {
      return added_.second + 1;
    }
  }

  inline void GetOwnPropertyNames(const JSGlobal* obj,
                                  Context* ctx,
                                  std::vector<Symbol>* vec,
                                  JSObject::EnumerationMode mode);

  void AllocateTable(Map* start) {
    assert(start);
    TargetTable* table = NULL;
    std::vector<Map*> old;
    Map* current = start;
    do {
      if (current->HasTable()) {
        // start with this table
        table = new (GC) TargetTable(*current->table_);
        break;
      }
      old.push_back(current);
      current = current->previous_;
    } while (current);
    if (!table) {
      // created table is not found
      table = new (GC) TargetTable;
    }
    for (std::vector<Map*>::const_reverse_iterator it = old.rbegin(),
         last = old.rend(); it != last; ++it) {
      table->insert((*it)->added_);
    }
    table_ = table;
  }

  bool HasTable() const {
    return table_;
  }

 private:
  Map* previous_;
  TargetTable* table_;
  Transitions transitions_;
  Deleted* deleted_;  // store deleted index for reuse
  std::pair<Symbol, std::size_t> added_;
  bool unique_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_MAP_H_
