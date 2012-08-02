// core is management class of radio noise GC
#ifndef IV_LV5_RADIO_CORE_FWD_H_
#define IV_LV5_RADIO_CORE_FWD_H_
#include <vector>
#include <deque>
#include <new>
#include <iv/detail/array.h>
#include <iv/noncopyable.h>
#include <iv/debug.h>
#include <iv/arith.h>
#include <iv/static_assert.h>
#include <iv/lv5/property_fwd.h>
#include <iv/lv5/radio/arena.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/radio/block.h>
#include <iv/lv5/radio/block_control_fwd.h>
namespace iv {
namespace lv5 {
class Context;
namespace radio {

static const std::size_t kInitialMarkStackSize =
    (4 * core::Size::KB) / sizeof(Cell*);  // NOLINT
static const std::size_t kMaxObjectSize = 512;
static const std::size_t kBlockControlStep = 16;
static const std::size_t kBlockControls = kMaxObjectSize / kBlockControlStep;

class BlockControl;
class Scope;

class Core : private core::Noncopyable<Core> {
 public:
  typedef std::array<BlockControl, kBlockControls> BlockControls;
  typedef std::vector<Cell*> MarkStack;
  typedef std::deque<Cell*> HandleStack;
  typedef std::deque<Cell*> PersistentStack;
  Core();

  ~Core();

  // allocate memory for radio::Cell
  template<typename T>
  Cell* Allocate() {
    IV_STATIC_ASSERT((std::is_base_of<Cell, T>::value));
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

  template<typename Scope>
  void EnterScope(Scope* scope) {
    scope->set_current(handles_.size());
  }

  template<typename Scope>
  void ExitScope(Scope* scope) {
    if (Cell* cell = scope->reserved()) {
      handles_.resize(scope->current() + 1);
      handles_.back() = cell;
    } else if (handles_.size() != scope->current()) {
      assert(handles_.size() > scope->current());
      handles_.resize(scope->current());
    }
  }

  template<typename Scope>
  void FenceScope(Scope* scope) {
    assert(scope->current() == handles_.size());
  }

  bool MarkCell(Cell* cell);

  bool MarkValue(JSVal val);

  bool MarkPropertyDescriptor(const PropertyDescriptor& desc);

  void Mark(Context* ctx);

  void Collect(Context* ctx);

  void ReleaseBlock(Block* block);

  struct Marker {
    explicit Marker(Core* core) : core_(core) { }
    void operator()(Cell* cell) {
      if (cell) {
        core_->MarkCell(cell);
      }
    }
    void operator()(const PropertyDescriptor& desc) {
      core_->MarkPropertyDescriptor(desc);
    }
    void operator()(JSVal val) {
      core_->MarkValue(val);
    }
    Core* core_;
  };

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

  Arena* working_;
  Block* free_blocks_;
  Cell* weak_maps_;
  HandleStack handles_;  // scoped handles
  MarkStack stack_;  // mark stack
  PersistentStack persistents_;  // persistent handles
  BlockControls controls_;  // blocks. first block is 8 bytes
};


} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_CORE_FWD_H_
