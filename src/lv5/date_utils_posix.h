#ifndef IV_LV5_DATE_UTILS_POSIX_H_
#define IV_LV5_DATE_UTILS_POSIX_H_
#include <sys/time.h>
#include <unistd.h>
#include "platform_math.h"
#include "canonicalized_nan.h"
namespace iv {
namespace lv5 {
namespace date {

inline double DaylightSavingTA(double utc) {
  // t is utc time
  if (core::IsNaN(utc)) {
    return utc;
  }
  const std::time_t current =
      static_cast<std::time_t>(std::floor(utc / kMsPerSecond));
  const std::tm* const t = std::localtime(&current);  // NOLINT
  if (t == NULL) {
    return iv::core::kNaN;
  }
  if (t->tm_isdst > 0) {
    return kMsPerHour;
  } else if (t->tm_isdst == 0) {
    return 0.0;
  } else {
    // fallback
    return DaylightSavingTAFallback(utc);
  }
}

inline double CurrentTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec + tv.tv_usec / 1000000.0) * 1000.0;
}

inline double HighResTime() {
  return CurrentTime();
}

} } }  // namespace iv::lv5::date
#endif  // IV_LV5_DATE_UTILS_POSIX_H_
