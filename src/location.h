#ifndef IV_LOCATION_H_
#define IV_LOCATION_H_
#include <cstddef>
#include "detail/type_traits.h"
#include "static_assert.h"
#include "platform.h"
namespace iv {
namespace core {

struct Location {
  std::size_t begin_position_;
  std::size_t end_position_;
  inline std::size_t begin_position() const {
    return begin_position_;
  }
  inline std::size_t end_position() const {
    return end_position_;
  }
  inline void set_begin_position(std::size_t begin) {
    begin_position_ = begin;
  }
  inline void set_end_position(std::size_t end) {
    end_position_ = end;
  }
};

#if defined(IV_COMPILER_GCC) && (IV_COMPILER_GCC >= 40300)
IV_STATIC_ASSERT(std::is_pod<Location>::value);
#endif

} }  // namespace iv::core
#endif  // IV_LOCATION_H_
