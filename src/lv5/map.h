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
#include <iostream>
#include <limits>
#include "notfound.h"
#include "lv5/jsobject.h"
#include "lv5/symbol.h"
#include "lv5/property.h"
#include "lv5/gc_template.h"
#include "lv5/context_utils.h"
namespace iv {
namespace lv5 {

class JSGlobal;

class Map : public gc {
 public:
  typedef GCHashMap<Symbol, std::size_t>::type TargetTable;
  typedef GCHashMap<Symbol, Map*>::type Transitions;

  class DeleteEntry {
   public:
    DeleteEntry(DeleteEntry* prev, std::size_t offset)
      : prev_(prev),
        offset_(offset) {
    }

    std::size_t offset() const {
      return offset_;
    }

    DeleteEntry* previous() const {
      return prev_;
    }

   private:
    DeleteEntry* prev_;
    std::size_t offset_;
  };

  class DeleteEntryHolder {
   public:
    DeleteEntryHolder() : entry_(NULL), size_(0) { }
    std::size_t size() const {
      return size_;
    }

    void Push(std::size_t offset) {
      entry_ = new (GC) DeleteEntry(entry_, offset);
      ++size_;
    }

    std::size_t Pop() {
      assert(entry_);
      const std::size_t res = entry_->offset();
      entry_ = entry_->previous();
      --size_;
      return res;
    }

    bool empty() const {
      return size_ == 0;
    }

   private:
    DeleteEntry* entry_;
    std::size_t size_;
  };

  struct UniqueTag { };

  static Map* NewUniqueMap(Context* ctx) {
    return new Map(UniqueTag());
  }

  static Map* New(Context* ctx) {
    return new Map();
  }

  bool IsUnique() const {
    return transitions_ == NULL;
  }

  std::size_t Get(Context* ctx, Symbol name) {
    const TargetTable::const_iterator it = table_.find(name);
    if (it != table_.end()) {
      return it->second;
    } else {
      return core::kNotFound;
    }
  }

  Map* DeletePropertyTransition(Context* ctx, Symbol name) {
    assert(GetSlotsSize() > 0);
    Map* map = NewUniqueMap(ctx, this);
    map->Delete(ctx, name);
    return map;
  }

  Map* AddPropertyTransition(Context* ctx, Symbol name, std::size_t* offset) {
    if (IsUnique()) {
      // extend this map with not transition
      std::size_t slot;
      if (!deleted_.empty()) {
        slot = deleted_.Pop();
      } else {
        slot = GetSlotsSize();
      }
      table_.insert(std::make_pair(name, slot));
      *offset = slot;
      return this;
    } else {
      assert(transitions_);
      const Transitions::const_iterator it = transitions_->find(name);
      if (it != transitions_->end()) {
        // found already created map. so, move to this
        *offset = it->second->added_;
        return it->second;
      }
      Map* map = New(ctx, this);
      if (!map->deleted_.empty()) {
        const std::size_t slot = map->deleted_.Pop();
        map->added_ = slot;
      } else {
        map->added_ = GetSlotsSize();
      }
      map->table_.insert(std::make_pair(name, map->added_));
      transitions_->insert(std::make_pair(name, map));
      *offset = map->added_;
      assert(map->GetSlotsSize() > map->added_);
      return map;
    }
  }

  std::size_t GetSlotsSize() const {
    return table_.size() + deleted_.size();
  }

  inline void GetOwnPropertyNames(const JSObject* obj,
                                  Context* ctx,
                                  std::vector<Symbol>* vec,
                                  JSObject::EnumerationMode mode);

  void MakeTransitionable(Context* ctx) {
    transitions_ = new (GC) Transitions();
  }

 private:
  static Map* NewUniqueMap(Context* ctx, Map* previous) {
    assert(previous);
    return new Map(previous, UniqueTag());
  }

  static Map* New(Context* ctx, Map* previous) {
    assert(previous);
    return new Map(previous);
  }

  explicit Map(Map* previous)
    : previous_(previous),
      table_(previous->table_),
      transitions_(new (GC) Transitions()),
      deleted_(previous->deleted_),
      added_(0) {
  }

  explicit Map()
    : previous_(NULL),
      table_(),
      transitions_(new (GC) Transitions()),
      deleted_(),
      added_(0) {
  }

  // empty start table
  // this is unique map. so only used in unique object, like
  // Object.prototype, Array.prototype, GlobalObject...
  Map(UniqueTag dummy)
    : previous_(NULL),
      table_(),
      transitions_(NULL),
      deleted_(),
      added_(0) {
  }

  Map(Map* previous, UniqueTag dummy)
    : previous_(previous),
      table_(previous->table_),
      transitions_(NULL),
      deleted_(previous->deleted_),
      added_(0) {
  }

  void Delete(Context* ctx, Symbol name) {
    const TargetTable::const_iterator it = table_.find(name);
    assert(it != table_.end());
    deleted_.Push(it->second);
    table_.erase(it);
  }

  Map* previous_;
  TargetTable table_;
  Transitions* transitions_;
  DeleteEntryHolder deleted_;
  std::size_t added_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_MAP_H_
