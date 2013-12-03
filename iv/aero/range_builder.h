#ifndef IV_AERO_RANGE_BUILDER_H_
#define IV_AERO_RANGE_BUILDER_H_
#include <vector>
#include <utility>
#include <algorithm>
#include <iv/noncopyable.h>
#include <iv/character.h>
#include <iv/sorted_vector.h>
#include <iv/aero/range.h>
#include <iv/aero/escape.h>
namespace iv {
namespace aero {

class RangeBuilder : private core::Noncopyable<RangeBuilder> {
 public:
  explicit RangeBuilder(bool ignore_case)
    : ignore_case_(ignore_case), sorted_(), ranges_() { }

  // range value is [start, last]
  typedef std::vector<Range> Ranges;
  typedef core::SortedVector<Range> SortedRanges;

  void Clear() {
    sorted_.clear();
    ranges_.clear();
  }

  bool IsIgnoreCase() const { return ignore_case_; }

  void AddRange(char16_t start, char16_t last, bool ignore_case) {
    if (start == last) {
      Add(start, ignore_case);
    } else {
      if (IsIgnoreCase() && ignore_case) {
        // TODO(Constellation): create char map is more fast?
        for (uint32_t ch = start; ch <= last; ++ch) {
          AddCharacterIgnoreCase(ch);
        }
      } else {
        sorted_.push_back(std::make_pair(start, last));
      }
    }
  }

  void AddOrEscaped(char16_t escaped, char16_t ch) {
    if (escaped == 0) {
      Add(ch, true);
    } else {
      AddEscape(escaped);
    }
  }

  void Add(char16_t ch, bool ignore_case) {
    if (IsIgnoreCase() && ignore_case) {
      AddCharacterIgnoreCase(ch);
    } else {
      sorted_.push_back(std::make_pair(ch, ch));
    }
  }

  const Ranges& GetEscapedRange(char16_t ch) {
    Clear();
    AddEscape(ch);
    return Finish();
  }

  const Ranges& Finish() {
    if (sorted_.empty()) {
      return ranges_;
    }
    SortedRanges::const_iterator it = sorted_.begin();
    const SortedRanges::const_iterator last = sorted_.end();
    Range current = *it;
    ++it;
    for (; it != last; ++it) {
      if ((current.second + 1) >= it->first) {
        current.second = std::max(current.second, it->second);
      } else {
        ranges_.push_back(current);
        current = *it;
      }
    }
    ranges_.push_back(current);
    return ranges_;
  }

  static bool IsValidRange(char16_t start, char16_t last) {
    return start <= last;
  }
 private:
  void AddEscape(char16_t escaped) {
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

  void AddCharacterIgnoreCase(char16_t ch) {
    const char16_t lu = core::character::ToLowerCase(ch);
    const char16_t uu = core::character::ToUpperCase(ch);
    if (lu == uu && lu == ch) {
      sorted_.push_back(std::make_pair(ch, ch));
    } else {
      sorted_.push_back(std::make_pair(lu, lu));
      sorted_.push_back(std::make_pair(uu, uu));
      sorted_.push_back(std::make_pair(ch, ch));
    }
  }

  template<typename Iter>
  void AddInvertedRanges(Iter it, Iter last) {
    char16_t start = 0x0000;
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
  SortedRanges sorted_;
  Ranges ranges_;
};

} }  // namespace iv::aero
#endif  // IV_AERO_RANGE_BUILDER_H_
