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
  static const uint32_t kMaxVectorSize = 10000;

  IndexedElements()
    : vector(),
      map(NULL),
      length_(0),
      dense_(true),
      writable_(true) {
  }

  SparseArrayMap* EnsureMap() {
    if (!map) {
      map = new (GC) SparseArrayMap();
    }
    return map;
  }

  void MakeSparse() {
    dense_ = false;
    SparseArrayMap* sparse = EnsureMap();
    uint32_t index = 0;
    for (DenseArrayVector::const_iterator it = vector.begin(),
         last = vector.end(); it != last; ++it, ++index) {
      if (!it->IsEmpty()) {
        sparse->insert(std::make_pair(index, StoredSlot(*it, ATTR::Object::Data())));
      }
    }
    vector.clear();
  }

  void MakeDense() {
    dense_ = true;
    map = NULL;
  }

  void MakeReadOnly() { writable_ = false; }
  bool dense() const { return dense_; }
  bool writable() const { return writable_; }

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
  static std::size_t DenseOffset() {
    return IV_OFFSETOF(IndexedElements, dense_);
  }
  static std::size_t WritableOffset() {
    return IV_OFFSETOF(IndexedElements, writable_);
  }

  DenseArrayVector vector;
  SparseArrayMap* map;
 private:
  uint32_t length_;
  bool dense_;
  bool writable_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_INDEXED_ELEMENTS_H_
