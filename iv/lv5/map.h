// Map object is class like object
// This classes are used for implementing Polymorphic Inline Cache.
//
// original paper is
//   http://cs.au.dk/~hosc/local/LaSC-4-3-pp243-281.pdf
//
#ifndef IV_LV5_MAP_H_
#define IV_LV5_MAP_H_
#include <gc/gc_cpp.h>
#include <iv/notfound.h>
#include <iv/debug.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/gc_template.h>
namespace iv {
namespace lv5 {

class MapBuilder;

class Map : public radio::HeapObject<radio::POINTER> {
 public:
  friend class MapBuilder;
  typedef GCHashMap<Symbol, std::size_t>::type TargetTable;

  class Transitions {
   public:
    enum {
      MASK_ENABLED = 1,
      MASK_UNIQUE_TRANSITION = 2,
      MASK_HOLD_SINGLE = 4,
      MASK_HOLD_TABLE = 8
    };

    typedef GCHashMap<Symbol, Map*>::type Table;

    explicit Transitions(bool enabled)
      : holder_(),
        flags_(enabled ? 1 : 0) { }

    bool IsEnabled() const {
      return flags_ & MASK_ENABLED;
    }

    Map* Find(Symbol name) {
      assert(IsEnabled());
      if (flags_ & MASK_HOLD_TABLE) {
        Table::const_iterator it = holder_.table->find(name);
        if (it != holder_.table->end()) {
          return it->second;
        }
      } else if (flags_ & MASK_HOLD_SINGLE) {
        if (holder_.pair.name == name) {
          return holder_.pair.map;
        }
      }
      return NULL;
    }

    void Insert(Symbol name, Map* map) {
      assert(IsEnabled());
      if (flags_ & MASK_HOLD_SINGLE) {
        Table* table = new(GC)Table();
        table->insert(std::make_pair(holder_.pair.name, holder_.pair.map));
        holder_.table = table;
        flags_ &= ~MASK_HOLD_SINGLE;
        flags_ |= MASK_HOLD_TABLE;
      }

      if (flags_ & MASK_HOLD_TABLE) {
        holder_.table->insert(std::make_pair(name, map));
      } else {
        holder_.pair.name = name;
        holder_.pair.map = map;
        flags_ |= MASK_HOLD_SINGLE;
      }
    }

    void EnableUniqueTransition() {
      assert(!IsEnabled());
      flags_ |= MASK_UNIQUE_TRANSITION;
    }

    bool IsEnabledUniqueTransition() const {
      return flags_ & MASK_UNIQUE_TRANSITION;
    }

   private:
    union {
      Table* table;
      struct {
        Symbol name;
        Map* map;
      } pair;
    } holder_;
    uint32_t flags_;
  };

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
      entry_ = new(GC)DeleteEntry(entry_, offset);
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

  static const std::size_t kMaxTransition = 32;

  static Map* NewUniqueMap(Context* ctx) {
    return new Map(UniqueTag());
  }

  static Map* New(Context* ctx) {
    return new Map();
  }

  std::size_t Get(Context* ctx, Symbol name) {
    if (!HasTable()) {
      if (!previous_) {
        // empty top table
        return core::kNotFound;
      }
      if (IsAddingMap()) {
        if (added_.first == name) {
          return added_.second;
        }
      }
      AllocateTable();
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
    return map;
  }

  Map* AddPropertyTransition(Context* ctx, Symbol name, std::size_t* offset) {
    if (IsUnique()) {
      // extend this map with no transition
      if (!HasTable()) {
        AllocateTable();
      }
      assert(HasTable());
      Map* map = NULL;
      if (transitions_.IsEnabledUniqueTransition()) {
        map = NewUniqueMap(ctx, this);
      } else {
        map = this;
      }
      assert(map->HasTable());
      std::size_t slot;
      if (!map->deleted_.empty()) {
        slot = map->deleted_.Pop();
      } else {
        slot = map->GetSlotsSize();
      }
      map->table_->insert(std::make_pair(name, slot));
      *offset = slot;
      return map;
    } else {
      // existing transition check
      if (Map* target = transitions_.Find(name)) {
        // found already created map. so, move to this
        *offset = target->added_.second;
        return target;
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
      transitions_.Insert(name, map);
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
                                  PropertyNamesCollector* collector,
                                  JSObject::EnumerationMode mode);

  void Flatten() {
    if (IsUnique()) {
      transitions_.EnableUniqueTransition();
    }
  }

  void MarkChildren(radio::Core* core) {
    if (previous_) {
      core->MarkCell(previous_);
    }
  }

 private:
  bool IsUnique() const {
    return !transitions_.IsEnabled();
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

  // empty not unique map
  Map()
    : previous_(NULL),
      table_(NULL),
      transitions_(true),
      deleted_(),
      added_(std::make_pair(symbol::kDummySymbol, core::kNotFound)),
      calculated_size_(0),
      transit_count_(0) {
  }

  explicit Map(Map* previous)
    : previous_(previous),
      table_(NULL),
      transitions_(true),
      deleted_(previous->deleted_),
      added_(std::make_pair(symbol::kDummySymbol, core::kNotFound)),
      calculated_size_(previous->GetSlotsSize()),
      transit_count_(0) {
  }

  // empty unique table
  explicit Map(UniqueTag dummy)
    : previous_(NULL),
      table_(NULL),
      transitions_(false),
      deleted_(),
      added_(std::make_pair(symbol::kDummySymbol, core::kNotFound)),
      calculated_size_(0),
      transit_count_(0) {
  }

  Map(Map* previous, UniqueTag dummy)
    : previous_(previous),
      table_((previous->IsUnique()) ? previous->table_ : NULL),
      transitions_(false),
      deleted_(previous->deleted_),
      added_(std::make_pair(symbol::kDummySymbol, core::kNotFound)),
      calculated_size_(previous->GetSlotsSize()),
      transit_count_(0) {
  }

  explicit Map(TargetTable* table)
    : previous_(NULL),
      table_(table),
      transitions_(true),
      deleted_(),
      added_(std::make_pair(symbol::kDummySymbol, core::kNotFound)),
      calculated_size_(GetSlotsSize()),
      transit_count_(0) {
  }


  // ObjectLiteral Map
  template<typename Iter>
  Map(Iter it, Iter last)
    : previous_(NULL),
      table_(new(GC)TargetTable(it, last)),
      transitions_(true),
      deleted_(),
      added_(std::make_pair(symbol::kDummySymbol, core::kNotFound)),
      calculated_size_(GetSlotsSize()),
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
        table_ = new(GC)TargetTable();
        break;
      } else {
        if (current->HasTable()) {
          table_ = new(GC)TargetTable(*current->table());
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

    table_->rehash(stack.size());
    for (std::vector<Map*>::const_reverse_iterator it = stack.rbegin(),
         last = stack.rend(); it != last; ++it) {
      assert((*it)->IsAddingMap());
      assert(table_->find((*it)->added_.first) == table_->end());
      table_->insert((*it)->added_);
    }
    assert(GetSlotsSize() == calculated_size_);
    previous_ = NULL;
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
  Transitions transitions_;
  DeleteEntryHolder deleted_;
  std::pair<Symbol, std::size_t> added_;
  std::size_t calculated_size_;
  std::size_t transit_count_;
};

class MapBuilder {
 public:
  MapBuilder(Context* ctx)
    : table_(new(GC)Map::TargetTable()) { }

  Map* Build() {
    return new Map(table_);
  }

  void Add(Symbol symbol, std::size_t index) {
    assert(Find(symbol) == core::kNotFound);
    table_->insert(std::make_pair(symbol, index));
  }

  std::size_t Add(Symbol symbol) {
    assert(Find(symbol) == core::kNotFound);
    const std::size_t index = table_->size();
    table_->insert(std::make_pair(symbol, index));
    return index;
  }

  std::size_t Find(Symbol symbol) const {
    const Map::TargetTable::const_iterator it = table_->find(symbol);
    if (it != table_->end()) {
      return it->second;
    } else {
      return core::kNotFound;
    }
  }

 private:
  Context* ctx_;
  Map::TargetTable* table_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_MAP_H_
