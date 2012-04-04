#ifndef IV_DATE_UTILS_POSIX_H_
#define IV_DATE_UTILS_POSIX_H_
#include <sys/time.h>
#include <unistd.h>
#include <iv/platform_math.h>
#include <iv/canonicalized_nan.h>
namespace iv {
namespace core {
namespace date {

inline double DaylightSavingTA(double utc) {
  // t is utc time
  if (core::math::IsNaN(utc)) {
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

inline const char* LocalTimeZone(double t) {
  if (core::math::IsNaN(t)) {
    return "";
  }
  const std::time_t tv = static_cast<time_t>(std::floor(t / kMsPerSecond));
  const struct std::tm* const tmp = std::localtime(&tv);  // NOLINT
  if (NULL == tmp) {
    return "";
  }
#if defined(IV_OS_CYGWIN)
  // cygwin not have tm_zone
  // but, TimeZone name is provided as tzname[0] in cygwin
  return tzname[0];
#else
  return tmp->tm_zone;
#endif
}

} } }  // namespace iv::core::date
#endif  // IV_DATE_UTILS_POSIX_H_
