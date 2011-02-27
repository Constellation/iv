#ifndef _IV_LV5_DATE_UTILS_H_
#define _IV_LV5_DATE_UTILS_H_
#include <ctime>
#include <cmath>
#include <cstring>
#include <tr1/cstdint>
#include <tr1/array>
#include "os_defines.h"
#include "conversions.h"
#include "utils.h"
namespace iv {
namespace lv5 {
namespace date {

static const int kHoursPerDay = 24;
static const int kMinutesPerHour = 60;
static const int kSecondsPerMinute = 60;

static const int kMsPerSecond = 1000;
static const int kMsPerMinute = kMsPerSecond * kSecondsPerMinute;
static const int kMsPerHour = kMsPerMinute * kMinutesPerHour;
static const int kMsPerDay = kMsPerHour * kHoursPerDay;

static const int64_t kEpochTime = 116444736000000000LL;

static const double kMaxTime = 8.64E15;

inline double Day(double t) {
  return std::floor(t / kMsPerDay);
}

inline double TimeWithinDay(double t) {
  const double res = std::fmod(t, kMsPerDay);
  if (res < 0) {
    return res + kMsPerDay;
  }
  return res;
}

inline int DaysInYear(int y) {
  return (((y % 100 == 0) ? y / 100 : y) % 4 == 0) ? 366 : 365;
}

inline double DaysFromYear(int y) {
  static const int kLeapDaysBefore1971By4Rule = 1970 / 4;
  static const int kExcludeLeapDaysBefore1971By100Rule = 1970 / 100;
  static const int kLeapDaysBefore1971By400Rule = 1970 / 400;

  const double year_minus_one = y - 1;
  const double years_to_add_by_4 =
      std::floor(year_minus_one / 4.0) - kLeapDaysBefore1971By4Rule;
  const double years_to_exclude_by_100 =
      std::floor(year_minus_one / 100.0) - kExcludeLeapDaysBefore1971By100Rule;
  const double years_to_add_by_400 =
      std::floor(year_minus_one / 400.0) - kLeapDaysBefore1971By400Rule;
  return 365.0 * (y - 1970) +
      years_to_add_by_4 -
      years_to_exclude_by_100 +
      years_to_add_by_400;
}

inline double TimeFromYear(int y) {
  return kMsPerDay * DaysFromYear(y);
}

// JSC's method
inline int YearFromTime(double t) {
  const int about = static_cast<int>(
      std::floor(t / (kMsPerDay * 365.2425)) + 1970);
  const double time = TimeFromYear(about);
  if (time > t) {
    return about - 1;
  } else if (TimeFromYear(about+1) <= t) {
    return about + 1;
  } else {
    return about;
  }
}

inline int IsLeapYear(double t) {
  return (DaysInYear(YearFromTime(t)) == 366) ? 1 : 0;
}

inline int DayWithinYear(double t) {
  return core::DoubleToInt32(Day(t) - DaysFromYear(YearFromTime(t)));
}

static const int kDaysMap[2][12] = {
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static const int kMonthMap[2][12] = {
  {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
  {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

static const std::tr1::array<const char*, 12> kMonths = { {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
} };

inline int MonthFromTime(double t) {
  int within = DayWithinYear(t);
  const int leap = IsLeapYear(t);
  for (int i = 0; i < 12; ++i) {
    within -= kDaysMap[leap][i];
    if (within < 0) {
      return i;
    }
  }
  UNREACHABLE();
  return 0;  // makes compiler happy
}

inline const char* MonthToString(double t) {
  assert(0 <= MonthFromTime(t) &&
         MonthFromTime(t) < static_cast<int>(kMonths.size()));
  return kMonths[MonthFromTime(t)];
}

inline int DateFromTime(double t) {
  int within = DayWithinYear(t);
  const int leap = IsLeapYear(t);
  for (int i = 0; i < 12; ++i) {
    within -= kDaysMap[leap][i];
    if (within < 0) {
      return within + kDaysMap[leap][i] + 1;
    }
  }
  UNREACHABLE();
  return 0;  // makes compiler happy
}

inline int MonthToDaysInYear(int month, int is_leap) {
  return kMonthMap[is_leap][month];
}

static const std::tr1::array<const char*, 7> kWeekDays = { {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
} };

inline int WeekDay(double t) {
  const int res = core::DoubleToInt32(std::fmod((Day(t) + 4), 7));
  if (res < 0) {
    return res + 7;
  }
  return res;
}

inline const char* WeekDayToString(double t) {
  assert(0 <= WeekDay(t) && WeekDay(t) < static_cast<int>(kWeekDays.size()));
  return kWeekDays[WeekDay(t)];
}

inline int32_t GetLocalTZA() {
  const std::time_t current = std::time(NULL);
  std::tm local;
  std::memcpy(&local, std::localtime(&current), sizeof(std::tm));  // NOLINT
  local.tm_sec = 0;
  local.tm_min = 0;
  local.tm_hour = 0;
  local.tm_mday = 1;
  local.tm_mon = 0;
  local.tm_wday = 0;
  local.tm_yday = 0;
  local.tm_isdst = 0;
  local.tm_year = 109;
  return (1230768000 - std::mktime(&local)) * 1000;
}

inline int32_t LocalTZA() {
  static const int32_t local_tza = GetLocalTZA();
  return local_tza;
}

double DaylightSavingTA(double t);

static const char* kNaNTimeZone = "";
inline const char* LocalTimeZone(double t) {
  if (std::isnan(t)) {
    return kNaNTimeZone;
  }
  const std::time_t tv = static_cast<time_t>(std::floor(t/kMsPerSecond));
  const struct std::tm* const tmp = std::localtime(&tv);  // NOLINT
  if (NULL == tmp) {
    return kNaNTimeZone;
  }
  return tmp->tm_zone;
}

inline double LocalTime(double t) {
  return t + LocalTZA() + DaylightSavingTA(t);
}

inline double UTC(double t) {
  const double local = LocalTZA();
  return t - local - DaylightSavingTA(t - local);
}

inline int HourFromTime(double t) {
  const int res = core::DoubleToInt32(
      std::fmod(std::floor(t / kMsPerHour), kHoursPerDay));
  if (res < 0) {
    return res + kHoursPerDay;
  }
  return res;
}

inline int MinFromTime(double t) {
  const int res = core::DoubleToInt32(
      std::fmod(std::floor(t / kMsPerMinute), kMinutesPerHour));
  if (res < 0) {
    return res + kMinutesPerHour;
  }
  return res;
}

inline int SecFromTime(double t) {
  const int res = core::DoubleToInt32(
      std::fmod(std::floor(t / kMsPerSecond), kSecondsPerMinute));
  if (res < 0) {
    return res + kSecondsPerMinute;
  }
  return res;
}

inline int MsFromTime(double t) {
  const int res = core::DoubleToInt32(
      std::fmod(t, kMsPerSecond));
  if (res < 0) {
    return res + kMsPerSecond;
  }
  return res;
}

inline double MakeTime(double hour, double min, double sec, double ms) {
  if (!std::isfinite(hour) ||
      !std::isfinite(min) ||
      !std::isfinite(sec) ||
      !std::isfinite(ms)) {
    return JSValData::kNaN;
  } else {
    return
        core::DoubleToInteger(hour) * kMsPerHour +
        core::DoubleToInteger(min) * kMsPerMinute +
        core::DoubleToInteger(sec) * kMsPerSecond +
        core::DoubleToInteger(ms);
  }
}

inline double DateToDays(int year, int month, int date) {
  if (month < 0) {
    month += 12;
    --year;
  }
  const double yearday = std::floor(DaysFromYear(year));
  const int monthday = MonthToDaysInYear(month,
                                         (DaysInYear(year) == 366 ? 1 : 0));
  assert((year >= 1970 && yearday >= 0) || (year < 1970 && yearday < 0));
  return yearday + monthday + date - 1;
}

inline double MakeDay(double year, double month, double date) {
  if (!std::isfinite(year) ||
      !std::isfinite(month) ||
      !std::isfinite(date)) {
    return JSValData::kNaN;
  } else {
    const int y = core::DoubleToInt32(year);
    const int m = core::DoubleToInt32(month);
    const int dt = core::DoubleToInt32(date);
    const int ym = y + m / 12;
    const int mn = m % 12;
    return DateToDays(ym, mn, dt);
  }
}

inline double MakeDate(double day, double time) {
  double res = day * kMsPerDay + time;
  if (std::abs(res) > kMaxTime) {
    return JSValData::kNaN;
  }
  return res;
}

inline double TimeClip(double time) {
  if (!std::isfinite(time)) {
    return JSValData::kNaN;
  }
  if (std::abs(time) > kMaxTime) {
    return JSValData::kNaN;
  }
  return core::DoubleToInteger(time);
}

} } }  // namespace iv::lv5::date
#ifdef OS_WIN
#include "lv5/date_utils_win.h"
#else
#include "lv5/date_utils_posix.h"
#endif  // OS_WIN
#endif  // _IV_LV5_DATE_UTILS_H_
