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
      stack_() {
    stack_.reserve(kInitialMarkStackSize);
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
    return GetTargetBlock(IV_ROUNDUP(sizeof(T), 2))->Allocate(this);
  }

 private:
  Block* GetTargetBlock(std::size_t size) {
    assert(size % 2 == 0);
    return NULL;
  }

  void Mark() { }

  void Sweep() { }

  void Drain() { }

  Arena* working_;
  std::vector<Cell*> handles_;  // scoped handles
  std::vector<Cell*> stack_;  // mark stack
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_H_
