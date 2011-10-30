#ifndef IV_LV5_RADIO_CORE_H_
#define IV_LV5_RADIO_CORE_H_
#include <algorithm>
#include "functor.h"
#include "lv5/context.h"
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
  std::size_t size = kBlockControlStep;
  for (BlockControls::iterator it = controls_.begin(),
       last = controls_.end(); it != last; ++it, size += kBlockControlStep) {
    assert(size <= kMaxObjectSize);
    it->Initialize(size);
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

inline bool Core::MarkValue(const JSVal& val) {
  return (val.IsCell()) ? MarkCell(val.cell()) : false;
}

inline bool Core::MarkPropertyDescriptor(const PropertyDescriptor& desc) {
  if (desc.IsDataDescriptor()) {
    return MarkValue(desc.AsDataDescriptor()->value());
  } else {
    assert(desc.IsAccessorDescriptor());
    const AccessorDescriptor* accs = desc.AsAccessorDescriptor();
    bool res = false;
    if (accs->get()) {
      res |= MarkCell(accs->get());
    }
    if (accs->set()) {
      res |= MarkCell(accs->set());
    }
    return res;
  }
}

inline void Core::CollectGarbage(Context* ctx) {
  // mark & sweep
  Mark(ctx);
  Collect(ctx);
}

inline void Core::Drain() {
  while (!stack_.empty()) {
    Cell* cell = stack_.back();
    stack_.pop_back();
    if (cell->color() == Color::GRAY) {
      cell->MarkChildren(this);
      cell->Coloring(Color::BLACK);
    }
  }
}

inline bool Core::MarkWeaks() {
  // TODO(Constellation): implement it
  bool newly_marked = false;
  return newly_marked;
}

inline void Core::MarkRoots(Context* ctx) {
  // mark context (and global data, vm stack)
  if (ctx) {
    // ctx->Mark(this);
  }
  // mark handles
  std::for_each(handles_.begin(), handles_.end(), Marker(this));
  // mark persistent handles
  std::for_each(persistents_.begin(), persistents_.end(), Marker(this));
}

inline void Core::Mark(Context* ctx) {
  // mark all roots
  MarkRoots(ctx);

  // see harmony proposal gc algorithm
  // http://wiki.ecmascript.org/doku.php?id=harmony:weak_maps#abstract_gc_algorithm
  // TODO(Constellation): stack all mark is faster? implement it.
  Drain();
  while (MarkWeaks()) {
    Drain();
  }
  assert(stack_.empty());
}

inline void Core::Collect(Context* ctx) {
  for (BlockControls::iterator it = controls_.begin(),
       last = controls_.end(); it != last; ++it) {
    it->Collect(this, ctx);
  }
}

inline void Core::ReleaseBlock(Block* block) {
  block->Release();
  block->set_next(free_blocks_);
  free_blocks_ = block;
}

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_H_
