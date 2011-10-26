#ifndef IV_LV5_RADIO_CORE_H_
#define IV_LV5_RADIO_CORE_H_
#include <algorithm>
#include "functor.h"
#include "lv5/radio/core_fwd.h"
#include "lv5/radio/scope.h"
#include "lv5/radio/block_control.h"
namespace iv {
namespace lv5 {
namespace radio {

inline Core::Core()
  : working_(NULL),
    free_blocks_(NULL),
    handles_(),
    stack_(),
    controls_() {
  stack_.reserve(kInitialMarkStackSize);
  AddArena();
  for (std::size_t i = 3; i < 16; ++i) {
    controls_[i - 3] = new BlockControl(1 << i);
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
  assert(control);
  return control->Allocate(this);
}

inline void Core::EnterScope(Scope* scope) {
  scope->set_current(handles_.size());
}

inline void Core::ExitScope(Scope* scope) {
  if (Cell* cell = scope->reserved()) {
    handles_.resize(scope->current() + 1);
    handles_.back() = cell;
  } else {
    handles_.resize(scope->current());
  }
}

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_H_
