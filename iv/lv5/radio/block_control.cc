#include <iv/lv5/context.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/block_control.h>
namespace iv {
namespace lv5 {
namespace radio {

Cell* BlockControl::Allocate(Core* core) {
  if (free_cells_) {
    Cell* target = free_cells_;
    free_cells_ = free_cells_->next();
    return target;
  }
  // assign block
  AllocateBlock(core);
  // and try once more
  return Allocate(core);
}

void BlockControl::Collect(Core* core, Context* ctx) {
  for (Block *prev = NULL, *block = block_; block;) {
    if (block->Collect(core, ctx, this)) {
      // when all objects are sweeped in block
      if (prev) {
        prev->set_next(block->next());
      } else {
        block_ = block->next();
      }
      Block* will_released = block;
      // enumeration
      block = block->next();
      core->ReleaseBlock(will_released);
    } else {
      // enumeration
      prev = block;
      block = block->next();
    }
  }
}

void BlockControl::AllocateBlock(Core* core) {
  assert(!free_cells_);
  Block* block = core->AllocateBlock(size_);
  block->set_next(block_);
  block_ = block;

  assert(block->IsUsed());

  // assign
  Block::iterator it = block->begin();
  const Block::const_iterator last = block->end();
  assert(it != last);
  free_cells_ = new(reinterpret_cast<void*>(&*it))Cell;
  Cell* prev = free_cells_;
  ++it;
  while (true) {
    if (it != last) {
      Cell* cell = new(reinterpret_cast<void*>(&*it))Cell;
      prev->set_next(cell);
      prev = cell;
      ++it;
    } else {
      prev->set_next(NULL);
      break;
    }
  }
}

void BlockControl::CollectCell(Core* core, Context* ctx, Cell* cell) {
  cell->~Cell();
  cell = new(static_cast<void*>(cell))Cell;
  assert(cell->color() == Color::CLEAR);
  cell->set_next(free_cells_);
  free_cells_ = cell;
}

} } }  // namespace iv::lv5::radio
