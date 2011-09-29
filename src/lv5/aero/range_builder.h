#ifndef IV_LV5_AERO_RANGE_BUILDER_H_
#define IV_LV5_AERO_RANGE_BUILDER_H_
#include <vector>
#include <utility>
#include <algorithm>
#include "noncopyable.h"
#include "character.h"
#include "lv5/aero/range.h"
#include "lv5/aero/escape.h"
namespace iv {
namespace lv5 {
namespace aero {

class RangeBuilder : core::Noncopyable<RangeBuilder> {
 public:
  explicit RangeBuilder(bool ignore_case)
    : ignore_case_(ignore_case), ranges_() { }

  // range value is [start, last]
  typedef std::vector<Range> Ranges;

  void Clear() {
    ranges_.clear();
  }

  bool IsIgnoreCase() const { return ignore_case_; }

  void AddRange(uint16_t start, uint16_t last, bool ignore_case) {
    if (start == last) {
      Add(start, ignore_case);
    } else {
      if (IsIgnoreCase() && ignore_case) {
        // TODO(Constellation): create char map is more fast?
        for (uint32_t ch = start; ch <= last; ++ch) {
          AddCharacterIgnoreCase(ch);
        }
      } else {
        ranges_.push_back(std::make_pair(start, last));
      }
    }
  }

  void AddOrEscaped(uint16_t escaped, uint16_t ch) {
    if (escaped == 0) {
      Add(ch, true);
    } else {
      AddEscape(escaped);
    }
  }

  void Add(uint16_t ch, bool ignore_case) {
    if (IsIgnoreCase() && ignore_case) {
      AddCharacterIgnoreCase(ch);
    } else {
      ranges_.push_back(std::make_pair(ch, ch));
    }
  }

  const Ranges& GetEscapedRange(uint16_t ch) {
    Clear();
    AddEscape(ch);
    return Finish();
  }

  const Ranges& Finish() {
    if (ranges_.empty()) {
      return ranges_;
    }
    Ranges result;
    std::sort(ranges_.begin(), ranges_.end());
    Ranges::const_iterator it = ranges_.begin();
    const Ranges::const_iterator last = ranges_.end();
    Range current = *it;
    ++it;
    for (; it != last; ++it) {
      if ((current.second + 1) >= it->first) {
        current.second = std::max(current.second, it->second);
      } else {
        result.push_back(current);
        current = *it;
      }
    }
    result.push_back(current);
    ranges_.swap(result);
    return ranges_;
  }

  static bool IsValidRange(uint16_t start, uint16_t last) {
    return start <= last;
  }
 private:
  void AddEscape(uint16_t escaped) {
    switch (escaped) {
      case 'd': {
        AddRanges(kDigitRanges.begin(), kDigitRanges.end());
        break;
      }
      case 'D': {
        AddInvertedRanges(kDigitRanges.begin(), kDigitRanges.end());
        break;
      }
      case 's': {
        AddRanges(kSpaceRanges.begin(), kSpaceRanges.end());
        break;
      }
      case 'S': {
        AddInvertedRanges(kSpaceRanges.begin(), kSpaceRanges.end());
        break;
      }
      case 'w': {
        AddRanges(kWordRanges.begin(), kWordRanges.end());
        break;
      }
      case 'W': {
        AddInvertedRanges(kWordRanges.begin(), kWordRanges.end());
        break;
      }
      case 'n': {
        AddRanges(kLineTerminatorRanges.begin(), kLineTerminatorRanges.end());
        break;
      }
      case '.': {
        AddInvertedRanges(kLineTerminatorRanges.begin(),
                          kLineTerminatorRanges.end());
        break;
      }
    }
  }

  void AddCharacterIgnoreCase(uint16_t ch) {
    const uint16_t lu = core::character::ToLowerCase(ch);
    const uint16_t uu = core::character::ToUpperCase(ch);
    if (lu == uu && lu == ch) {
      ranges_.push_back(std::make_pair(ch, ch));
    } else {
      ranges_.push_back(std::make_pair(lu, lu));
      ranges_.push_back(std::make_pair(uu, uu));
      ranges_.push_back(std::make_pair(ch, ch));
    }
  }

  template<typename Iter>
  void AddInvertedRanges(Iter it, Iter last) {
    uint16_t start = 0x0000;
    for (; it != last; ++it) {
      AddRange(start, it->first - 1, false);
      start = it->second + 1;
    }
    AddRange(start, 0xFFFF, false);
  }

  template<typename Iter>
  void AddRanges(Iter it, Iter last) {
    for (; it != last; ++it) {
      AddRange(it->first, it->second, false);
    }
  }

  bool ignore_case_;
  Ranges ranges_;
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_RANGE_BUILDER_H_
