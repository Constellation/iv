#ifndef IV_LV5_RADIO_BLOCK_CONTROL_FWD_H_
#define IV_LV5_RADIO_BLOCK_CONTROL_FWD_H_
#include <new>
#include "noncopyable.h"
namespace iv {
namespace lv5 {

class Context;

namespace radio {

class Core;
class Cell;
class Block;

class BlockControl : private core::Noncopyable<BlockControl> {
 public:
  BlockControl()
    : size_(0), block_(NULL), free_cells_(NULL) { }

  void Initialize(std::size_t size) {
    size_ = size;
  }

  Cell* Allocate(Core* core);

  void Collect(Core* core, Context* ctx);

  void CollectCell(Core* core, Context* ctx, Cell* cell);

 private:
  void AllocateBlock(Core* core);

  std::size_t size_;
  Block* block_;
  Cell* free_cells_;
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_CONTROL_FWD_H_
