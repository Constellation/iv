#ifndef IV_LV5_RADIO_BLOCK_H_
#define IV_LV5_RADIO_BLOCK_H_
#include <new>
#include "detail/cstdint.h"
#include "utils.h"
#include "debug.h"
#include "noncopyable.h"
#include "lv5/radio/cell.h"
// this is radio::Block
// control space and memory block
namespace iv {
namespace lv5 {
namespace radio {

static const std::size_t kBlockSize = core::Size::KB * 4;

class Block : private core::Noncopyable<Block> {
 public:
  typedef Block this_type;
  typedef std::size_t size_type;
  typedef char* memory_type;
  typedef const char* const_memory_type;

  Block(size_type object_size)
    : object_size_(object_size) {
    assert((reinterpret_cast<uintptr_t>(this) % kBlockSize) == 0);
  }

  size_type GetControlSize() const {
    return IV_ROUNDUP(sizeof(this_type), object_size_);
  }

  memory_type begin() {
    return reinterpret_cast<memory_type>(this) + GetControlSize();
  }

  const_memory_type begin() const {
    return reinterpret_cast<const_memory_type>(this) + GetControlSize();
  }

  memory_type end() {
    return reinterpret_cast<memory_type>(this + 1);
  }

  const_memory_type end() const {
    return reinterpret_cast<const_memory_type>(this + 1);
  }

  template<typename Func>
  void Iterate(Func func) {
    for (memory_type data = begin(), last = end();
         data < last; data += object_size_) {
      func(reinterpret_cast<Cell*>(data));
    }
  }

  template<typename Func>
  void Iterate(Func func) const {
    for (const_memory_type data = begin(), last = end();
         data < last; data += object_size_) {
      func(reinterpret_cast<const Cell*>(data));
    }
  }

  void* Allocate() {
    return NULL;
  }

  // destruct and chain to free list
  struct Drainer {
    Drainer(Block* block) : block_(block) { }
    void operator()(Cell* cell) {
      cell->~Cell();
      block_->Chain(cell);
    }
    Block* block_;
  };

  void Drain() {
    Iterate(Drainer(this));
  }

  void Chain(Cell* cell) {
    cell->set_next(top_);
    top_ = cell;
  }

  void DestroyAllCells() {
  }

 private:
  size_type object_size_;
  Cell* top_;
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_H_
