#ifndef IV_LV5_RADIO_BLOCK_CONTROL_H_
#define IV_LV5_RADIO_BLOCK_CONTROL_H_
namespace iv {
namespace lv5 {
namespace radio {

class BlockControl : private core::Noncopyable<BlockControl> {
 public:
  BlockControl() { }

  Initialize(std::size_t size) {
    size_ = size;
  }

  Cell* Allocate(Core* core) {
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

 private:
  void AllocateBlock(Core* core) {
    assert(!free_cells_);
    Block* block = core->AllocateBlock(size_);
    block->set_next(block_);
    block_ = block;

    // assign
    Block::iterator it = block->begin();
    const Block::const_iterator last = block->end();
    free_cells_ = *it;
    while (true) {
      Cell* cell = *it;
      ++it;
      if (it != last) {
        cell->set_next(NULL);
        break;
      } else {
        cell->set_next(*it);
      }
    }
  }

  std::size_t size_;
  Block* block_;
  Cell* free_cells_;
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_CONTROL_H_
