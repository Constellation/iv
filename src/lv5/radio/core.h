// core is management class of radio noise GC
#ifndef IV_LV5_RADIO_CORE_H_
#define IV_LV5_RADIO_CORE_H_
#include <vector>
#include "noncopyable.h"
#include "debug.h"
#include "lv5/radio/arena.h"
#include "lv5/radio/cell.h"
namespace iv {
namespace lv5 {
namespace radio {

static const std::size_t kInitialMarkStackSize = 64;

class Core : private core::Noncopyable<Core> {
 public:
  Core()
    : working_(new Arena()),
      handles_(),
      stack_(),
      controls_() {
    stack_.reserve(kInitialMarkStackSize);
    for (std::size_t i = 3, i < 16; ++i) {
      controls_.Initialize(1 << i);
    }
  }

  ~Core() {
    // Destroy All Arenas
    for (Arena* arena = working_; arena;) {
      Arena* next = arena->prev();
      arena->DestroyAllCells();
      arena->~Arena();
      arena = next;
    }
  }

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
    return GetBlockControl(CLP2(sizeof(T)))->Allocate(this);
  }

 private:
  BlockControl* GetBlockControl(std::size_t size) {
    assert(size % 2 == 0);
    // first block bytes is 8
    const std::size_t n = NLZ64(size) - 3;
    return &controls_[n];
  }

  void Mark() { }

  void Sweep() { }

  void Drain() { }

  Arena* working_;
  std::vector<Cell*> handles_;  // scoped handles
  std::vector<Cell*> stack_;  // mark stack
  std::array<BlockControl, 13> controls_;  // blocks. first block is 8 bytes
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_H_
