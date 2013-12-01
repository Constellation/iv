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
#include <iv/lv5/jsobject_fwd.h>
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

  typedef core::QHashMap<Symbol, Entry, symbol::KeyTraits, GCAlloc> TargetTable;

  class Transitions {
   public:
    enum {
      MASK_ENABLED = 1,
      MASK_UNIQUE_TRANSITION = 2,
      MASK_HOLD_SINGLE = 4,
      MASK_HOLD_TABLE = 8,
      MASK_INDEXED = 16
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

      // traits
      static unsigned hash(const Key& val) {
        return Hasher()(val);
      }
      static bool equals(const Key& lhs, const Key& rhs) {
        return lhs == rhs;
      }
      // because of Array index
      static Key null() {
        static const Key kDummy = {
          symbol::kDummySymbol,
          0
        };
        return kDummy;
      }
    };

    typedef core::QHashMap<Key, Map*, Key, GCAlloc> Table;

    explicit Transitions(bool enabled, bool indexed) : holder_(), flags_(0) {
      set_enabled(enabled);
      set_indexed(indexed);
    }

    bool IsEnabled() const { return flags_ & MASK_ENABLED; }
    bool IsIndexed() const { return flags_ & MASK_INDEXED; }

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
      return nullptr;
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

    void set_enabled(bool enabled) {
      if (enabled) {
        flags_ |= MASK_ENABLED;
      } else {
        flags_ &= ~MASK_ENABLED;
      }
    }

    void set_indexed(bool indexed) {
      if (indexed) {
        flags_ |= MASK_INDEXED;
      } else {
        flags_ &= ~MASK_INDEXED;
      }
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
    DeleteEntryHolder() : entry_(nullptr), size_(0) { }

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

  static const std::size_t kMaxTransition = 32;

  static Map* New(Context* ctx, Map* previous) {
    assert(previous);
    return new Map(previous, false);
  }

  static Map* NewUniqueMap(Context* ctx, Map* previous) {
    assert(previous);
    return new Map(previous, true);
  }

  static Map* New(Context* ctx, JSObject* prototype, bool indexed) {
    return new Map(prototype, false, indexed);
  }

  static Map* NewUniqueMap(Context* ctx, JSObject* prototype, bool indexed) {
    return new Map(prototype, true, indexed);
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
      Map* map = nullptr;
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
      map->table_->Lookup(name, true)->second = entry;
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
        if (symbol::IsPublicSymbol(it->first)) {
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

  JSObject* prototype() const { return prototype_; }

  Map* ChangePrototypeWithNoTransition(JSObject* prototype) {
    prototype_ = prototype;
    return this;
  }

  Map* ChangeExtensibleTransition(Context* ctx) {
    return NewUniqueMap(ctx, this);
  }

  Map* ChangePrototypeTransition(Context* ctx, JSObject* prototype) {
    if (IsUnique()) {
      // extend this map with no transition
      Map* map = nullptr;
      if (transitions_.IsEnabledUniqueTransition()) {
        map = NewUniqueMap(ctx, this);
      } else {
        map = this;
      }
      map->prototype_ = prototype;
      return map;
    }

    // stop transition
    Map* map = NewUniqueMap(ctx, this);
    // go to above unique path
    return map->ChangePrototypeTransition(ctx, prototype);
  }

  Map* ChangeIndexedTransition(Context* ctx) {
    if (IsUnique()) {
      // extend this map with no transition
      Map* map = nullptr;
      if (transitions_.IsEnabledUniqueTransition()) {
        map = NewUniqueMap(ctx, this);
      } else {
        map = this;
      }
      map->transitions_.set_indexed(true);
      return map;
    }

    // stop transition
    Map* map = NewUniqueMap(ctx, this);
    // go to above unique path
    return map->ChangeIndexedTransition(ctx);
  }

  static std::size_t PrototypeOffset() { return IV_OFFSETOF(Map, prototype_); }

  inline bool IsIndexed() const { return transitions_.IsIndexed(); }

  inline bool IsShaped() const {
    // we can use this map id as shape or not
    return !IsUnique() || transitions_.IsEnabled();
  }

 private:
  bool HasTable() const {
    return table_;
  }

  Map(JSObject* prototype, bool unique, bool indexed)
    : prototype_(prototype),
      previous_(nullptr),
      table_(nullptr),
      transitions_(!unique, indexed),
      deleted_(),
      added_(std::make_pair(symbol::kDummySymbol, Entry::NotFound())),
      calculated_size_(0),
      transit_count_(0) {
  }

  Map(Map* previous, bool unique)
    : prototype_(previous->prototype()),
      previous_(previous),
      table_((unique && previous->IsUnique()) ? previous->table_ : nullptr),
      transitions_(!unique, previous->transitions_.IsIndexed()),
      deleted_(previous->deleted_),
      added_(std::make_pair(symbol::kDummySymbol, Entry::NotFound())),
      calculated_size_(previous->GetSlotsSize()),
      transit_count_(0) {
  }

  explicit Map(TargetTable* table, JSObject* prototype, bool unique, bool indexed)
    : prototype_(prototype),
      previous_(nullptr),
      table_(table),
      transitions_(!unique, indexed),
      deleted_(),
      added_(std::make_pair(symbol::kDummySymbol, Entry::NotFound())),
      calculated_size_(GetSlotsSize()),
      transit_count_(0) {
  }

  // ObjectLiteral Map
  template<typename Iter>
  Map(Iter it, Iter last)
    : prototype_(nullptr),
      previous_(nullptr),
      table_(new(GC)TargetTable(it, last)),
      transitions_(true, false),
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

    // table_->rehash(stack.size());
    for (std::vector<Map*>::const_reverse_iterator it = stack.rbegin(),
         last = stack.rend(); it != last; ++it) {
      assert((*it)->IsAddingMap());
      assert(table_->find((*it)->added_.first) == table_->end());
      assert((*it)->added_.first != symbol::kDummySymbol);
      table_->insert((*it)->added_);
    }
    assert(GetSlotsSize() == calculated_size_);
    previous_ = nullptr;
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

  JSObject* prototype_;
  Map* previous_;
  TargetTable* table_;
  Transitions transitions_;
  DeleteEntryHolder deleted_;
  std::pair<Symbol, Entry> added_;
  uint32_t calculated_size_;
  uint32_t transit_count_;
};

} }  // namespace iv::lv5
#include "jsobject_interface.h"
#endif  // IV_LV5_MAP_H_
