#ifndef IV_LV5_RADIO_CORE_H_
#define IV_LV5_RADIO_CORE_H_
#include <algorithm>
#include "functor.h"
#include "lv5/radio/core_fwd.h"
#include "lv5/radio/block_control.h"
namespace iv {
namespace lv5 {
namespace radio {

inline Core::Core()
  : working_(new Arena()),
    handles_(),
    stack_(),
    controls_() {
  stack_.reserve(kInitialMarkStackSize);
  for (std::size_t i = 3; i < 16; ++i) {
    controls_[i] = new BlockControl(1 << i);
  }
}

inline Core::~Core() {
  // Destroy All Arenas
  for (Arena* arena = working_; arena;) {
    Arena* next = arena->prev();
    arena->DestroyAllBlocks();
    arena->~Arena();
    arena = next;
  }
  std::for_each(controls_.begin(), controls_.end(),
                core::Deleter<BlockControl>());
}

inline Cell* Core::AllocateFrom(BlockControl* control) {
  return control->Allocate(this);
}

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_H_
