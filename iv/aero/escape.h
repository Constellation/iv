#ifndef IV_AERO_ESCAPE_H_
#define IV_AERO_ESCAPE_H_
#include <utility>
#include <iv/detail/array.h>
#include <iv/aero/range.h>
namespace iv {
namespace aero {

// escape ranges from V8 irregexp
static const std::array<Range, 10> kSpaceRanges = { {
  std::make_pair(0x0009, 0x000D),
  std::make_pair(0x0020, 0x0020),
  std::make_pair(0x00A0, 0x00A0),
  std::make_pair(0x1680, 0x1680),
  std::make_pair(0x180E, 0x180E),
  std::make_pair(0x2000, 0x200A),
  std::make_pair(0x2028, 0x2029),
  std::make_pair(0x202F, 0x202F),
  std::make_pair(0x205F, 0x205F),
  std::make_pair(0x3000, 0x3000)
} };

static const std::array<Range, 4> kWordRanges = { {
  std::make_pair('0', '9'),
  std::make_pair('A', 'Z'),
  std::make_pair('_', '_'),
  std::make_pair('a', 'z')
} };


static const std::array<Range, 1> kDigitRanges = { {
  std::make_pair('0', '9'),
} };

static const std::array<Range, 3> kLineTerminatorRanges = { {
  std::make_pair(0x000A, 0x000A),
  std::make_pair(0x000D, 0x000D),
  std::make_pair(0x2028, 0x2029)
} };

} }  // namespace iv::aero
#endif  // IV_AERO_ESCAPE_H_
