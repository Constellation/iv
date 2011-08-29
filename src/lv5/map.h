// Map object is class like object
// This classes are used for implementing Polymorphic Inline Cache.
//
// original paper is
//   http://cs.au.dk/~hosc/local/LaSC-4-3-pp243-281.pdf
//
#ifndef _IV_LV5_MAP_H_
#define _IV_LV5_MAP_H_
#include <gc/gc_cpp.h>
#include "notfound.h"
#include "debug.h"
#include "lv5/jsobject.h"
#include "lv5/symbol.h"
#include "lv5/gc_template.h"
namespace iv {
namespace lv5 {

class Map : public gc {
 public:
  typedef GCHashMap<Symbol, std::size_t>::type TargetTable;
  typedef GCHashMap<Symbol, Map*>::type Transitions;

  static const std::size_t kMaxTransition = 64;

  static Map* NewUniqueMap(Context* ctx) {
    return new Map(UniqueTag());
  }

  static Map* New(Context* ctx) {
    return new Map();
  }

  std::size_t Get(Context* ctx, Symbol name) {
    if (!AllocateTableIfNeeded()) {
      return core::kNotFound;
    }
    const TargetTable::const_iterator it = table_->find(name);
    if (it != table_->end()) {
      return it->second;
    } else {
      return core::kNotFound;
    }
  }

  Map* DeletePropertyTransition(Context* ctx, Symbol name) {
    assert(GetSlotsSize() > 0);
    Map* map = NewUniqueMap(ctx, this);
    if (!map->HasTable()) {
      map->AllocateTable();
    }
    map->Delete(ctx, name);
    assert(GetSlotsSize() == map->GetSlotsSize());
    return map;
  }

  Map* AddPropertyTransition(Context* ctx, Symbol name, std::size_t* offset) {
    if (IsUnique()) {
      // extend this map with not transition
      if (!HasTable()) {
        AllocateTable();
      }
      assert(HasTable());
      std::size_t slot;
      if (!deleted_.empty()) {
        slot = deleted_.Pop();
      } else {
        slot = GetSlotsSize();
      }
      table_->insert(std::make_pair(name, slot));
      *offset = slot;
      return this;
    } else {
      assert(transitions_);
      // existing transition check
      const Transitions::const_iterator it = transitions_->find(name);
      if (it != transitions_->end()) {
        // found already created map. so, move to this
        *offset = it->second->added_.second;
        return it->second;
      }

      if (transit_count_ > kMaxTransition) {
        // stop transition
        Map* map = NewUniqueMap(ctx, this);
        // go to above unique path
        return map->AddPropertyTransition(ctx, name, offset);
      }

      Map* map = New(ctx, this);
      if (!map->deleted_.empty()) {
        const std::size_t slot = map->deleted_.Pop();
        map->added_ = std::make_pair(name, slot);
        map->calculated_size_ = GetSlotsSize();
      } else {
        map->added_ = std::make_pair(name, GetSlotsSize());
        map->calculated_size_ = GetSlotsSize() + 1;
      }
      map->transit_count_ = transit_count_ + 1;
      transitions_->insert(std::make_pair(name, map));
      *offset = map->added_.second;
      assert(map->GetSlotsSize() > map->added_.second);
      return map;
    }
  }

  std::size_t GetSlotsSize() const {
    return (table_) ? table_->size() + deleted_.size() : calculated_size_;
  }

  inline void GetOwnPropertyNames(const JSObject* obj,
                                  Context* ctx,
                                  std::vector<Symbol>* vec,
                                  JSObject::EnumerationMode mode);

  void MakeTransitionable(Context* ctx) {
    transitions_ = new (GC) Transitions();
  }

 private:
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

  bool IsUnique() const {
    return transitions_ == NULL;
  }

  bool HasTable() const {
    return table_;
  }

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
      table_(NULL),
      transitions_(new (GC) Transitions()),
      deleted_(previous->deleted_),
      added_(std::make_pair(Symbol(), core::kNotFound)),
      calculated_size_(previous->GetSlotsSize()),
      transit_count_(0) {
  }

  Map()
    : previous_(NULL),
      table_(NULL),
      transitions_(new (GC) Transitions()),
      deleted_(),
      added_(std::make_pair(Symbol(), core::kNotFound)),
      calculated_size_(0),
      transit_count_(0) {
  }

  // empty start table
  // this is unique map. so only used in unique object, like
  // Object.prototype, Array.prototype, GlobalObject...
  Map(UniqueTag dummy)
    : previous_(NULL),
      table_(NULL),
      transitions_(NULL),
      deleted_(),
      added_(std::make_pair(Symbol(), core::kNotFound)),
      calculated_size_(0),
      transit_count_(0) {
  }

  Map(Map* previous, UniqueTag dummy)
    : previous_(previous),
      table_((previous->IsUnique()) ? previous->table_ : NULL),
      transitions_(NULL),
      deleted_(previous->deleted_),
      added_(std::make_pair(Symbol(), core::kNotFound)),
      calculated_size_(previous->GetSlotsSize()),
      transit_count_(0) {
  }

  bool IsAddingMap() const {
    return added_.second != core::kNotFound;
  }

  bool AllocateTableIfNeeded() {
    if (!HasTable()) {
      if (!previous_) {
        // empty top table
        return false;
      }
      AllocateTable();
    }
    return true;
  }

  void AllocateTable() {
    std::vector<Map*> stack;
    stack.reserve(8);
    assert(!HasTable());
    assert(previous_ || IsUnique());
    if (IsAddingMap()) {
      stack.push_back(this);
    }
    Map* current = previous_;
    while (true) {
      if (!current) {
        table_ = new (GC) TargetTable();
        break;
      } else {
        if (current->HasTable()) {
          table_ = new (GC) TargetTable(*current->table());
          break;
        } else {
          if (current->IsAddingMap()) {
            stack.push_back(current);
          }
        }
      }
      current = current->previous_;
    }
    assert(table_);

    for (std::vector<Map*>::const_reverse_iterator it = stack.rbegin(),
         last = stack.rend(); it != last; ++it) {
      assert((*it)->IsAddingMap());
      assert(table_->find((*it)->added_.first) == table_->end());
      table_->insert((*it)->added_);
    }
    assert(GetSlotsSize() == calculated_size_);
  }

  TargetTable* table() const {
    return table_;
  }

  void Delete(Context* ctx, Symbol name) {
    assert(HasTable());
    const TargetTable::const_iterator it = table_->find(name);
    assert(it != table_->end());
    deleted_.Push(it->second);
    table_->erase(it);
  }

  Map* previous_;
  TargetTable* table_;
  Transitions* transitions_;
  DeleteEntryHolder deleted_;
  std::pair<Symbol, std::size_t> added_;
  std::size_t calculated_size_;
  std::size_t transit_count_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_MAP_H_
