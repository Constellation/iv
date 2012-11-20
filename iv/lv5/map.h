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
#include <iv/lv5/attributes.h>
#include <iv/lv5/gc_template.h>
namespace iv {
namespace lv5 {

class MapBuilder;

class Map : public radio::HeapObject<radio::POINTER> {
 public:
  friend class MapBuilder;

  class Entry {
   public:
    uint32_t offset;
    Attributes::Safe attributes;  // Property Descriptor's attributes
    Entry() : offset(0), attributes(Attributes::Safe::NotFound()) { }
    Entry(uint32_t o, Attributes::Safe a) : offset(o), attributes(a) { }
    bool IsNotFound() const { return attributes.IsNotFound(); }
    static Entry NotFound() { return Entry(); }
  };

  typedef GCHashMap<Symbol, Entry>::type TargetTable;

  class Transitions {
   public:
    enum {
      MASK_ENABLED = 1,
      MASK_UNIQUE_TRANSITION = 2,
      MASK_HOLD_SINGLE = 4,
      MASK_HOLD_TABLE = 8
    };

    struct Key {
      Symbol name;
      Attributes::Raw attributes;

      inline friend bool operator==(Key lhs, Key rhs) {
        return lhs.name == rhs.name && lhs.attributes == rhs.attributes;
      }

      inline friend bool operator!=(Key lhs, Key rhs) {
        return !(lhs == rhs);
      }

      struct Hasher {
        std::size_t operator()(Key val) const {
          return
              std::hash<Symbol>()(val.name) +
              std::hash<uint32_t>()(val.attributes);
        }
      };

      static Key Create(Symbol name, Attributes::Safe attributes) {
        Key key;
        key.name = name;
        key.attributes = attributes.raw();
        return key;
      }
    };

    typedef GCHashMap<Key, Map*, Key::Hasher>::type Table;

    explicit Transitions(bool enabled) : holder_(), flags_(enabled ? 1 : 0) { }

    bool IsEnabled() const { return flags_ & MASK_ENABLED; }

    Map* Find(Symbol name, Attributes::Safe attributes) {
      assert(IsEnabled());
      const Key key = Key::Create(name, attributes);
      if (flags_ & MASK_HOLD_TABLE) {
        Table::const_iterator it = holder_.table->find(key);
        if (it != holder_.table->end()) {
          return it->second;
        }
      } else if (flags_ & MASK_HOLD_SINGLE) {
        if (holder_.pair.key == key) {
          return holder_.pair.map;
        }
      }
      return NULL;
    }

    void Insert(Symbol name, Attributes::Safe attributes, Map* map) {
      assert(IsEnabled());
      assert(name != symbol::kDummySymbol);
      const Key key = Key::Create(name, attributes);
      if (flags_ & MASK_HOLD_SINGLE) {
        Table* table = new(GC)Table();
        table->insert(std::make_pair(holder_.pair.key, holder_.pair.map));
        holder_.table = table;
        flags_ &= ~MASK_HOLD_SINGLE;
        flags_ |= MASK_HOLD_TABLE;
      }

      if (flags_ & MASK_HOLD_TABLE) {
        holder_.table->insert(std::make_pair(key, map));
      } else {
        holder_.pair.key = key;
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
        Key key;
        Map* map;
      } pair;
    } holder_;
    uint8_t flags_;
  };

  class DeleteEntry {
   public:
    DeleteEntry(DeleteEntry* prev, uint32_t offset)
      : prev_(prev),
        offset_(offset) {
    }
    uint32_t offset() const { return offset_; }
    DeleteEntry* previous() const { return prev_; }
   private:
    DeleteEntry* prev_;
    uint32_t offset_;
  };

  class DeleteEntryHolder {
   public:
    DeleteEntryHolder() : entry_(NULL), size_(0) { }

    void Push(uint32_t offset) {
      entry_ = new(GC)DeleteEntry(entry_, offset);
      ++size_;
    }

    uint32_t Pop() {
      assert(entry_);
      const uint32_t res = entry_->offset();
      entry_ = entry_->previous();
      --size_;
      return res;
    }

    std::size_t size() const { return size_; }

    bool empty() const { return size_ == 0; }

   private:
    DeleteEntry* entry_;
    uint32_t size_;
  };

  struct UniqueTag { };

  static const std::size_t kMaxTransition = 32;

  static Map* NewUniqueMap(Context* ctx) {
    return new Map(UniqueTag());
  }

  static Map* NewUniqueMap(Context* ctx, Map* previous) {
    assert(previous);
    return new Map(previous, UniqueTag());
  }

  static Map* New(Context* ctx) {
    return new Map();
  }

  // If unique map is provided, copy this.
  static Map* NewFromPoint(Context* ctx, Map* map) {
    if (map->IsUnique()) {
      return NewUniqueMap(ctx, map);
    }
    return map;
  }

  std::size_t StorageCapacity() const {
    return core::NextCapacity(GetSlotsSize());
  }

  Entry Get(Context* ctx, Symbol name) {
    if (!HasTable()) {
      if (!previous_) {
        // empty top table
        return Entry::NotFound();
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
      return Entry::NotFound();
    }
  }

  // TODO(Constellation) we should make uncacheable
  Map* DeletePropertyTransition(Context* ctx, Symbol name) {
    assert(GetSlotsSize() > 0);
    Map* map = NewUniqueMap(ctx, this);
    if (!map->HasTable()) {
      map->AllocateTable();
    }
    map->Delete(ctx, name);
    return map;
  }

  Map* ChangeAttributesTransition(Context* ctx, Symbol name, Attributes::Safe attributes) {
    Map* map = NewUniqueMap(ctx, this);
    if (!map->HasTable()) {
      map->AllocateTable();
    }
    map->ChangeAttributes(name, attributes);
    return map;
  }

  Map* AddPropertyTransition(Context* ctx, Symbol name,
                             Attributes::Safe attributes, uint32_t* offset) {
    Entry entry;
    entry.attributes = attributes;

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
      if (!map->deleted_.empty()) {
        entry.offset = map->deleted_.Pop();
      } else {
        entry.offset = map->GetSlotsSize();
      }
      assert(name != symbol::kDummySymbol);
      map->table_->insert(std::make_pair(name, entry));
      *offset = entry.offset;
      return map;
    }

    // existing transition check
    if (Map* target = transitions_.Find(name, attributes)) {
      // found already created map. so, move to this
      *offset = target->added_.second.offset;
      return target;
    }

    if (transit_count_ > kMaxTransition) {
      // stop transition
      Map* map = NewUniqueMap(ctx, this);
      // go to above unique path
      return map->AddPropertyTransition(ctx, name, attributes, offset);
    }

    Map* map = New(ctx, this);

    if (!map->deleted_.empty()) {
      const uint32_t slot = map->deleted_.Pop();
      map->added_ = std::make_pair(name, Entry(slot, attributes));
      map->calculated_size_ = GetSlotsSize();
    } else {
      map->added_ = std::make_pair(name, Entry(GetSlotsSize(), attributes));
      map->calculated_size_ = GetSlotsSize() + 1;
    }

    map->transit_count_ = transit_count_ + 1;
    transitions_.Insert(name, attributes, map);
    *offset = map->added_.second.offset;
    assert(map->GetSlotsSize() > map->added_.second.offset);
    return map;
  }

  std::size_t GetSlotsSize() const {
    return (table_) ? table_->size() + deleted_.size() : calculated_size_;
  }

  void GetOwnPropertyNames(PropertyNamesCollector* collector, bool include) {
    if (AllocateTableIfNeeded()) {
      for (TargetTable::const_iterator it = table_->begin(),
           last = table_->end(); it != last; ++it) {
        if (symbol::IsPrivateSymbol(it->first)) {
          continue;
        }
        if (include || it->second.attributes.IsEnumerable()) {
          collector->Add(it->first, it->second.offset);
        }
      }
    }
  }

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

  bool IsUnique() const {
    return !transitions_.IsEnabled();
  }
 private:
  bool HasTable() const {
    return table_;
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
      added_(std::make_pair(symbol::kDummySymbol, Entry::NotFound())),
      calculated_size_(0),
      transit_count_(0) {
  }

  explicit Map(Map* previous)
    : previous_(previous),
      table_(NULL),
      transitions_(true),
      deleted_(previous->deleted_),
      added_(std::make_pair(symbol::kDummySymbol, Entry::NotFound())),
      calculated_size_(previous->GetSlotsSize()),
      transit_count_(0) {
  }

  // empty unique table
  explicit Map(UniqueTag dummy)
    : previous_(NULL),
      table_(NULL),
      transitions_(false),
      deleted_(),
      added_(std::make_pair(symbol::kDummySymbol, Entry::NotFound())),
      calculated_size_(0),
      transit_count_(0) {
  }

  Map(Map* previous, UniqueTag dummy)
    : previous_(previous),
      table_((previous->IsUnique()) ? previous->table_ : NULL),
      transitions_(false),
      deleted_(previous->deleted_),
      added_(std::make_pair(symbol::kDummySymbol, Entry::NotFound())),
      calculated_size_(previous->GetSlotsSize()),
      transit_count_(0) {
  }

  explicit Map(TargetTable* table)
    : previous_(NULL),
      table_(table),
      transitions_(true),
      deleted_(),
      added_(std::make_pair(symbol::kDummySymbol, Entry::NotFound())),
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
      added_(std::make_pair(symbol::kDummySymbol, Entry::NotFound())),
      calculated_size_(GetSlotsSize()),
      transit_count_(0) {
  }

  bool IsAddingMap() const { return added_.first != symbol::kDummySymbol; }

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
      assert((*it)->added_.first != symbol::kDummySymbol);
      table_->insert((*it)->added_);
    }
    assert(GetSlotsSize() == calculated_size_);
    previous_ = NULL;
  }

  TargetTable* table() const { return table_; }

  void Delete(Context* ctx, Symbol name) {
    assert(HasTable());
    const TargetTable::const_iterator it = table_->find(name);
    assert(it != table_->end());
    deleted_.Push(it->second.offset);
    table_->erase(it);
  }

  void ChangeAttributes(Symbol name, Attributes::Safe attributes) {
    assert(HasTable());
    const TargetTable::iterator it = table_->find(name);
    assert(it != table_->end());
    it->second.attributes = attributes;
  }

  Map* previous_;
  TargetTable* table_;
  Transitions transitions_;
  DeleteEntryHolder deleted_;
  std::pair<Symbol, Entry> added_;
  uint32_t calculated_size_;
  uint32_t transit_count_;
};

class MapBuilder {
 public:
  explicit MapBuilder(Context* ctx)
    : table_(new(GC)Map::TargetTable()) { }

  Map* Build() { return new Map(table_); }

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
};

} }  // namespace iv::lv5
#endif  // IV_LV5_MAP_H_
