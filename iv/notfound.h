#ifndef IV_NOTFOUND_H_
#define IV_NOTFOUND_H_
#include <limits>
namespace iv {
namespace core {

static const uint32_t kNotFound32 = std::numeric_limits<uint32_t>::max();
static const uint64_t kNotFound64 = std::numeric_limits<uint64_t>::max();
static const uint32_t kNotFound = kNotFound32;

} }  // namespace iv::core
#endif  // IV_NOTFOUND_H_
