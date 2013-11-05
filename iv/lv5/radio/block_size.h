#ifndef IV_LV5_RADIO_BLOCK_SIZE_H_
#define IV_LV5_RADIO_BLOCK_SIZE_H_
namespace iv {
namespace lv5 {
namespace radio {
namespace detail_block_size {

template<std::size_t x>
struct Is2Power {
  static const bool value = x > 1 && (x & (x - 1)) == 0;
};

}  // namespace detail_block_size

class Block;

static const std::size_t kBlockSize = core::Size::KB * 4;
static const uintptr_t kBlockMask = ~static_cast<uintptr_t>(kBlockSize - 1);

static_assert(
    detail_block_size::Is2Power<kBlockSize>::value, "must be 2^n size");

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_SIZE_H_
