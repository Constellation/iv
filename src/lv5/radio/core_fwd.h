// core is management class of radio noise GC
#ifndef IV_LV5_RADIO_CORE_FWD_H_
#define IV_LV5_RADIO_CORE_FWD_H_
#include <vector>
#include <new>
#include "detail/array.h"
#include "noncopyable.h"
#include "debug.h"
#include "arith.h"
#include "static_assert.h"
#include "lv5/radio/arena.h"
#include "lv5/radio/cell.h"
#include "lv5/radio/block.h"
namespace iv {
namespace lv5 {
namespace radio {

static const std::size_t kInitialMarkStackSize = 64;

class BlockControl;
class Scope;

class Core : private core::Noncopyable<Core> {
 public:
  Core();

  ~Core();

  void AddArena() {
    assert(!free_blocks_);
    working_ = new Arena(working_);
    // assign
    Arena::iterator it = working_->begin();
    const Arena::const_iterator last = working_->end();
    assert(it != last);
    free_blocks_ = &*it;
    while (true) {
      Block* block = &*it;
      ++it;
      if (it != last) {
        block->set_next(&*it);
      } else {
        block->set_next(NULL);
        break;
      }
    }
  }

  void CollectGarbage() {
  }

  // allocate memory for radio::Cell
  template<typename T>
  Cell* Allocate() {
    typedef std::is_base_of<Cell, T> cond;
    IV_STATIC_ASSERT(cond::value);
    IV_STATIC_ASSERT(sizeof(T) >= 8);
    IV_STATIC_ASSERT(sizeof(T) <= (1 << 18));
    return AllocateFrom(GetBlockControl<core::detail::CLP2<sizeof(T)>::value>());
  }

  Block* AllocateBlock(std::size_t size) {
    if (free_blocks_) {
      Block* block = free_blocks_;
      free_blocks_ = free_blocks_->next();
      return new(block)Block(size);
    }
    AddArena();
    return AllocateBlock(size);
  }

  void EnterScope(Scope* scope);
  void ExitScope(Scope* scope);

 private:
  Cell* AllocateFrom(BlockControl* control);

  template<std::size_t N>
  BlockControl* GetBlockControl() {
    // first block bytes is 8
    IV_STATIC_ASSERT(N % 2 == 0);
    IV_STATIC_ASSERT(core::detail::NTZ<N>::value >= 3);
    IV_STATIC_ASSERT(core::detail::NTZ<N>::value <= 15);
    return controls_[core::detail::NTZ<N>::value - 3];
  }

  void Mark() { }

  void Sweep() { }

  void Drain() { }

  Arena* working_;
  Block* free_blocks_;
  std::vector<Cell*> handles_;  // scoped handles
  std::vector<Cell*> stack_;  // mark stack
  std::array<BlockControl*, 13> controls_;  // blocks. first block is 8 bytes
};


} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_FWD_H_
