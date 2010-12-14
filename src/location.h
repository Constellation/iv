#ifndef _IV_LOCATION_H_
#define _IV_LOCATION_H_
#include <cstddef>
#include <tr1/type_traits>
#include "static_assert.h"
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

#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 3)
IV_STATIC_ASSERT(std::tr1::is_pod<Location>::value);
#endif

} }  // namespace iv::core
#endif  // _IV_LOCATION_H_
