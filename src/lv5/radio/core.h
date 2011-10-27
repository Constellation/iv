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
    weak_maps_(NULL),
    handles_(),
    stack_(),
    persistents_(),
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
    assert(handles_.size() > scope->current());
    handles_.resize(scope->current());
  }
}

inline bool Core::MarkCell(Cell* cell) {
  if (cell->color() == Color::WHITE) {
    cell->Coloring(Color::GRAY);
    stack_.push_back(cell);
    return true;
  }
  return false;
}

inline void Core::Mark(Context* ctx) {
  // mark phase start

  // mark all roots
  {
    // mark context (and global data, vm stack)
    // ctx->Mark(this);
    // mark handles
    std::for_each(handles_.begin(), handles_.end(), Marker(this));
    // mark persistent handles
    std::for_each(persistents_.begin(), persistents_.end(), Marker(this));
  }

  // see harmony proposal gc algorithm
  // http://wiki.ecmascript.org/doku.php?id=harmony:weak_maps#abstract_gc_algorithm

  // TODO(Constellation): stack all mark is faster? implement it.
  while (!stack_.empty()) {
    Cell* cell = stack_.back();
    stack_.pop_back();
    if (cell->color() == Color::GRAY) {
      cell->MarkChildren(this);
      cell->Coloring(Color::BLACK);
    }
  }
}

inline void Core::Sweep(Context* ctx) {
}

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_H_
