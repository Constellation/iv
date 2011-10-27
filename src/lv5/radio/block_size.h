#ifndef IV_LV5_RADIO_BLOCK_SIZE_H_
#define IV_LV5_RADIO_BLOCK_SIZE_H_
#include "static_assert.h"
#include "arith.h"
namespace iv {
namespace lv5 {
namespace radio {

class Block;

static const std::size_t kBlockSize = core::Size::KB * 4;
static const uintptr_t kBlockMask = ~static_cast<uintptr_t>(kBlockSize - 1);

// must be 2^n size
IV_STATIC_ASSERT((1 << core::detail::NTZ<kBlockSize>::value) == kBlockSize);

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_BLOCK_SIZE_H_
