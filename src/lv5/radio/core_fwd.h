// core is management class of radio noise GC
#ifndef IV_LV5_RADIO_CORE_FWD_H_
#define IV_LV5_RADIO_CORE_FWD_H_
#include <vector>
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

class Core : private core::Noncopyable<Core> {
 public:
  Core();

  ~Core();

  void AddArena() {
    working_ = new Arena(working_);
  }

  void ShrinkArena() {
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
    return AllocateFrom(GetBlockControl(core::CLP2(sizeof(T))));
  }

  Block* AllocateBlock(std::size_t size) {
    return NULL;
  }

 private:
  Cell* AllocateFrom(BlockControl* control);

  BlockControl* GetBlockControl(std::size_t size) {
    assert(size % 2 == 0);
    // first block bytes is 8
    const std::size_t n = core::NTZ64(size) - 3;
    return controls_[n];
  }

  void Mark() { }

  void Sweep() { }

  void Drain() { }

  Arena* working_;
  std::vector<Cell*> handles_;  // scoped handles
  std::vector<Cell*> stack_;  // mark stack
  std::array<BlockControl*, 13> controls_;  // blocks. first block is 8 bytes
};


} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_FWD_H_
