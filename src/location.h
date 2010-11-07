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
};

IV_STATIC_ASSERT(std::tr1::is_pod<Location>::value);

} }  // namespace iv::core
#endif  // _IV_LOCATION_H_
