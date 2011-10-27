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
#include "lv5/radio/block_control_fwd.h"
namespace iv {
namespace lv5 {
class Context;
namespace radio {

static const std::size_t kInitialMarkStackSize = 64;
static const std::size_t kMaxObjectSize = 512;
static const std::size_t kBlockControlStep = 16;
static const std::size_t kBlockControls = kMaxObjectSize / kBlockControlStep;

class BlockControl;
class Scope;

class Core : private core::Noncopyable<Core> {
 public:
  typedef std::array<BlockControl, kBlockControls> BlockControls;
  Core();

  ~Core();

  // allocate memory for radio::Cell
  template<typename T>
  Cell* Allocate() {
    typedef std::is_base_of<Cell, T> cond;
    IV_STATIC_ASSERT(cond::value);
    IV_STATIC_ASSERT(AlignOf(T) <= kBlockControlStep);
    return AllocateFrom(
        GetBlockControl<IV_ROUNDUP(sizeof(T), kBlockControlStep)>());
  }

  // GC trigger
  void CollectGarbage(Context* ctx);

  Block* AllocateBlock(std::size_t size) {
    if (free_blocks_) {
      Block* block = free_blocks_;
      free_blocks_ = free_blocks_->next();
      return new(block)Block(size);
    }
    AddArena();
    return AllocateBlock(size);
  }

  void ChainToScope(Cell* cell) {
    handles_.push_back(cell);
  }

  void EnterScope(Scope* scope);

  void ExitScope(Scope* scope);

  bool MarkCell(Cell* cell);

  void Mark(Context* ctx);

  void Collect(Context* ctx);

  void ReturnBlock(Block* block);

 private:
  void AddArena() {
    assert(!free_blocks_);
    working_ = new Arena(working_);
    // assign
    Arena::iterator it = working_->begin();
    const Arena::const_iterator last = working_->end();
    assert(it != last);
    Block* prev = free_blocks_ = &*it;
    prev->Release();
    ++it;
    while (true) {
      if (it != last) {
        Block* block = &*it;
        block->Release();
        prev->set_next(block);
        prev = block;
        ++it;
      } else {
        prev->set_next(NULL);
        break;
      }
    }
  }

  void MarkRoots(Context* ctx);

  bool MarkWeaks();

  void Drain();

  Cell* AllocateFrom(BlockControl* control);

  template<std::size_t N>
  BlockControl* GetBlockControl() {
    // first block bytes is 8
    IV_STATIC_ASSERT(N % 2 == 0);
    IV_STATIC_ASSERT(N <= kMaxObjectSize);
    IV_STATIC_ASSERT(((N + 1) / kBlockControlStep) < kBlockControls);
    return &controls_[(N + 1) / kBlockControlStep];
  }

  struct Marker {
    Marker(Core* core) : core_(core) { }
    void operator()(Cell* cell) {
      core_->MarkCell(cell);
    }
    Core* core_;
  };

  Arena* working_;
  Block* free_blocks_;
  Cell* weak_maps_;
  std::vector<Cell*> handles_;  // scoped handles
  std::vector<Cell*> stack_;  // mark stack
  std::vector<Cell*> persistents_;  // persistent handles
  BlockControls controls_;  // blocks. first block is 8 bytes
};


} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_FWD_H_
