// structures for indexed elements in JSObject
#ifndef IV_LV5_INDEXED_ELEMENTS_H_
#define IV_LV5_INDEXED_ELEMENTS_H_
#include <iv/utils.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval_fwd.h>
#include <iv/lv5/storage.h>
namespace iv {
namespace lv5 {

class IndexedElements {
 public:
  typedef GCHashMap<uint32_t, StoredSlot>::type SparseArrayMap;
  typedef Storage<JSVal> DenseArrayVector;

  enum Flags {
    FLAG_DENSE = 1,
    FLAG_WRITABLE = 2
  };

  // 256 * n
  static const uint32_t kMaxVectorSize = 1024UL << 6;

  IndexedElements()
    : vector(),
      map(NULL),
      length_(0),
      flags_(FLAG_DENSE | FLAG_WRITABLE) {
  }

  SparseArrayMap* EnsureMap() {
    if (!map) {
      map = new (GC) SparseArrayMap();
    }
    return map;
  }

  void MakeSparse() {
    flags_ &= ~static_cast<uint32_t>(FLAG_DENSE);
    SparseArrayMap* sparse = EnsureMap();
    uint32_t index = 0;
    for (DenseArrayVector::const_iterator it = vector.begin(),
         last = vector.end(); it != last; ++it, ++index) {
      if (!it->IsEmpty()) {
        sparse->insert(std::make_pair(index, StoredSlot(*it, ATTR::Object::Data())));
      }
    }
    std::fill(vector.begin(), vector.end(), JSEmpty);
  }

  void MakeDense() {
    flags_ |= static_cast<uint32_t>(FLAG_DENSE);
    map = NULL;
  }

  void MakeReadOnly() { flags_ &= ~static_cast<uint32_t>(FLAG_WRITABLE); }
  bool dense() const { return flags_ & FLAG_DENSE; }
  bool writable() const { return flags_ & FLAG_WRITABLE; }

  uint32_t length() const { return length_; }
  void set_length(uint32_t len) { length_ = len; }

  static std::size_t VectorOffset() {
    return IV_OFFSETOF(IndexedElements, vector);
  }
  static std::size_t MapOffset() {
    return IV_OFFSETOF(IndexedElements, map);
  }
  static std::size_t LengthOffset() {
    return IV_OFFSETOF(IndexedElements, length_);
  }
  static std::size_t FlagsOffset() {
    return IV_OFFSETOF(IndexedElements, flags_);
  }

  DenseArrayVector vector;
  SparseArrayMap* map;
 private:
  uint32_t length_;
  uint32_t flags_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_INDEXED_ELEMENTS_H_
