#ifndef _IV_DIGIT_ITERATOR_H_
#define _IV_DIGIT_ITERATOR_H_
#include <iterator>
#include "detail/cstdint.h"
#include "character.h"
#include "conversions_digit.h"
namespace iv {
namespace core {

template<typename Iterator>
class DigitIterator : public std::iterator<std::forward_iterator_tag, int> {
 public:
  typedef DigitIterator<Iterator> this_type;

  DigitIterator(int radix, Iterator start, Iterator end)
    : radix_(radix),
      start_(start),
      end_(end),
      mask_(0),
      digit_(0) {
    assert(radix > 0);
    assert((radix & (radix - 1)) == 0);
    Next();
  }

  DigitIterator()
    : radix_(),
      start_(),
      end_(),
      mask_(0),
      digit_() {
    Finalize();
  }

  Iterator base() const {
    return start_;
  }

  int operator*() const {
    assert(mask_ != 0);
    return (digit_ & mask_) != 0;
  }

  bool operator==(const this_type& rhs) const {
    return (mask_ == 0 && rhs.mask_ == 0) ||
        ((radix_ == rhs.radix_) &&
         (start_ == rhs.start_) &&
         (mask_ == rhs.mask_) &&
         (digit_ == rhs.digit_));
  }

  bool operator!=(const this_type& rhs) const {
      return !(operator==(rhs));
  }

  this_type& operator++() {
    Next();
    return *this;
  }

  this_type operator++(int) {
    const this_type temp(*this);
    Next();
    return temp;
  }

 private:
  void Next() {
    mask_ >>= 1;
    if (mask_ == 0) {
      // get next digit
      if (start_ == end_) {
        Finalize();
        return;
      }
      const int c = *start_++;
      assert(character::IsHexDigit(c));
      digit_ = HexValue(c);
      mask_ = radix_ >> 1;
    }
  }

  void Finalize() {
    mask_ = 0;
  }

  const int radix_;
  Iterator start_;
  const Iterator end_;
  int mask_;
  int digit_;
};

} }  // namespace iv::core
#endif  // _IV_DIGIT_ITERATOR_H_
