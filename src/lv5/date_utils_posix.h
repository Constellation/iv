#ifndef _IV_LV5_DATE_UTILS_POSIX_H_
#define _IV_LV5_DATE_UTILS_POSIX_H_
#include <sys/time.h>
#include <unistd.h>
namespace iv {
namespace lv5 {
namespace date {

inline double DaylightSavingTA(double t) {
  if (std::isnan(t)) {
    return t;
  }
  const std::time_t current = core::DoubleToInt64(t);
  if (current == t) {
    const std::tm* const tmp = std::localtime(&current);  // NOLINT
    if (tmp->tm_isdst > 0) {
      return kMsPerHour;
    }
  } else {
    // Daylight Saving Time
    // from    2 AM the first Sunday in April
    // through 2 AM the last Sunday in October
    double target = t - LocalTZA();
    const int year = YearFromTime(target);
    const int leap = IsLeapYear(target);

    double start = TimeFromYear(year);
    double end = start;


    // goto April 1st
    start += static_cast<double>(MonthToDaysInYear(3, leap)) * kMsPerDay;
    // goto the first Sunday in April
    while (WeekDay(start) != 0) {
      start += kMsPerDay;
    }

    // goto Octobar 30th
    end += static_cast<double>((MonthToDaysInYear(9, leap) + 30)) * kMsPerDay;
    // goto the last Sunday in Octobar
    while (WeekDay(end) != 0) {
      end -= kMsPerDay;
    }

    target -= 2 * kMsPerHour;

    if (start <= target && target <= end) {
      return kMsPerHour;
    }
  }
  return 0.0;
}

inline double CurrentTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

} } }  // namespace iv::lv5::date
#endif  // _IV_LV5_DATE_UTILS_POSIX_H_
