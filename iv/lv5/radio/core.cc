#include <algorithm>
#include <iv/functor.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/context.h>
#include <iv/lv5/radio/core.h>
#include <iv/lv5/radio/scope.h>
#include <iv/lv5/radio/block_control.h>
namespace iv {
namespace lv5 {
namespace radio {

Core::Core()
  : working_(nullptr),
    free_blocks_(nullptr),
    weak_maps_(nullptr),
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

Core::~Core() {
  // Destroy All Arenas
  for (Arena* arena = working_; arena;) {
    Arena* next = arena->prev();
    arena->DestroyAllBlocks();
    arena->~Arena();
    arena = next;
  }
}

Cell* Core::AllocateFrom(BlockControl* control) {
  assert(control);
  return control->Allocate(this);
}

bool Core::MarkCell(Cell* cell) {
  if (cell->color() == Color::WHITE) {
    cell->Coloring(Color::GRAY);
    stack_.push_back(cell);
    return true;
  }
  return false;
}

bool Core::MarkValue(JSVal val) {
  return (val.IsCell()) ? MarkCell(val.cell()) : false;
}

bool Core::MarkPropertyDescriptor(const PropertyDescriptor& desc) {
  if (desc.IsData()) {
    return MarkValue(desc.AsDataDescriptor()->value());
  } else {
    assert(desc.IsAccessor());
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

void Core::CollectGarbage(Context* ctx) {
  // mark & sweep
  Mark(ctx);
  Collect(ctx);
}

void Core::Drain() {
  while (!stack_.empty()) {
    Cell* cell = stack_.back();
    stack_.pop_back();
    if (cell->color() == Color::GRAY) {
      cell->MarkChildren(this);
      cell->Coloring(Color::BLACK);
    }
  }
}

bool Core::MarkWeaks() {
  // TODO(Constellation): implement it
  bool newly_marked = false;
  return newly_marked;
}

void Core::MarkRoots(Context* ctx) {
  // mark context (and global data, vm stack)
  if (ctx) {
    // ctx->Mark(this);
  }
  // mark handles
  std::for_each(handles_.begin(), handles_.end(), Marker(this));
  // mark persistent handles
  std::for_each(persistents_.begin(), persistents_.end(), Marker(this));
}

void Core::Mark(Context* ctx) {
  // mark all roots
  MarkRoots(ctx);

  // see harmony proposal gc algorithm
  // http://wiki.ecmascript.org/doku.php?id=harmony:weak_maps#abstract_gc_algorithm
  // TODO(Constellation): stack all mark is faster? implement it.
  do {
    Drain();
  } while (MarkWeaks());
  assert(stack_.empty());
}

void Core::Collect(Context* ctx) {
  for (BlockControls::iterator it = controls_.begin(),
       last = controls_.end(); it != last; ++it) {
    it->Collect(this, ctx);
  }
}

void Core::ReleaseBlock(Block* block) {
  block->Release();
  block->set_next(free_blocks_);
  free_blocks_ = block;
}

} } }  // namespace iv::lv5::radio
