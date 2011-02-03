#ifndef _IV_LV5_RUNTIME_DATE_H_
#define _IV_LV5_RUNTIME_DATE_H_
#include <ctime>
#include <cmath>
#include <cstring>
#include <tr1/cstdint>
#include <tr1/array>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "utils.h"
#include "conversions.h"
#include "jsdate.h"
#include "jsstring.h"
#include "jsval.h"
namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

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
  const int about = static_cast<int>(std::floor(t / (kMsPerDay * 365.2425)) + 1970);
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

static const char* kMonths[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

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
  assert(0 <= MonthFromTime(t) && MonthFromTime(t) <= 11);
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

static const char* kWeekDays[7] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

inline int WeekDay(double t) {
  const int res = core::DoubleToInt32(std::fmod((Day(t) + 4), 7));
  if (res < 0) {
    return res + 7;
  }
  return res;
}

inline const char* WeekDayToString(double t) {
  assert(0 <= WeekDay(t) && WeekDay(t) <= 6);
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

#ifdef WIN32
inline std::time_t FileTimeToUnixTime(const FILETIME& ft) {
  LARGE_INTEGER i;
  i.LowPart = ft.dwLowDateTime;
  i.HighPart = ft.dwHighDateTime;
  return (i.QuadPart - kEpochTime) / 10 / 1000000;
}

inline std::time_t SystemTimeToUnixTime(const SYSTEMTIME& st) {
  FILETIME ft;
  ::SystemTimeToFileTime(&st, &ft);
  return FileTimeToUnixTime(ft);
}

inline double DaylightSavingTA(double t) {
  // http://msdn.microsoft.com/en-us/library/ms724421
  if (std::isnan(t)) {
    return t;
  }
  TIME_ZONE_INFORMATION tzi;
  const DWORD r = ::GetTimeZoneInformation(&tzi);
  switch (r) {
    case TIME_ZONE_ID_STANDARD:
    case TIME_ZONE_ID_DAYLIGHT: {
      if (tzi.StandardDate.wMonth == 0 ||
          tzi.DaylightDate.wMonth == 0) {
        break;
      }

      const std::time_t ts = SystemTimeToUnixTime(tzi.StandardDate);
      const std::time_t td = SystemTimeToUnixTime(tzi.DaylightDate);

      if (td <= t && t <= ts) {
        return - tzi.DaylightBias * (60 * kMsPerSecond);
      } else {
        return 0.0;
      }
    }
    case TIME_ZONE_ID_UNKNOWN: {
      // Daylight Saving Time not used in this time zone
      return 0.0;
    }
  }
  return 0.0;
}
#else
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
    start += MonthToDaysInYear(3, leap) * kMsPerDay;
    // goto the first Sunday in April
    while (WeekDay(start) != 0) {
      start += kMsPerDay;
    }

    // goto Octobar 30th
    end += (MonthToDaysInYear(9, leap) + 30) * kMsPerDay;
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
#endif

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
  const int monthday = MonthToDaysInYear(month, (DaysInYear(year) == 366 ? 1 : 0));
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

#ifdef WIN32
inline double CurrentTime() {
  FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);
  return FileTimeToUnixTime(ft);
}
#else
inline double CurrentTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}
#endif

}  // namespace iv::lv5::runtime::detail

// section 15.9.2.1
//   Date([year[, month[, date[, hours[, minutes[, seconds[, ms]]]])
// section 15.9.3.1
//   new Date(year, month[, date[, hours[, minutes[, seconds[, ms]]]])
inline JSVal DateConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    const std::size_t args_size = args.size();
    Context* const ctx = args.ctx();

    if (args_size == 0) {
      // section 15.9.3.3 new Date()
      return JSDate::New(
          ctx, detail::TimeClip(detail::CurrentTime() * 1000.0));
    }

    if (args_size == 1) {
      // section 15.9.3.2 new Date(value)
      const JSVal v = args[0].ToPrimitive(ctx, Hint::NONE, ERROR(error));
      if (v.IsString()) {
        // TODO(Constellation) parse implementation required
        return JSUndefined;
      } else {
        const double V = v.ToNumber(ctx, ERROR(error));
        return JSDate::New(
            ctx, detail::TimeClip(V));
      }
    }

    // following case, args_size > 1
    double y = args[0].ToNumber(ctx, ERROR(error));
    const double m = args[1].ToNumber(ctx, ERROR(error));

    double dt;
    if (args_size > 2) {
      dt = args[2].ToNumber(ctx, ERROR(error));
    } else {
      dt = 1;
    }

    double h;
    if (args_size > 3) {
      h = args[3].ToNumber(ctx, ERROR(error));
    } else {
      h = 0;
    }

    double min;
    if (args_size > 4) {
      min = args[4].ToNumber(ctx, ERROR(error));
    } else {
      min = 0;
    }

    double s;
    if (args_size > 5) {
      s = args[5].ToNumber(ctx, ERROR(error));
    } else {
      s = 0;
    }

    double milli;
    if (args_size > 6) {
      milli = args[6].ToNumber(ctx, ERROR(error));
    } else {
      milli = 0;
    }

    if (!std::isnan(y)) {
      const double integer = core::DoubleToInteger(y);
      if (0 <= integer && integer <= 99) {
        y = 1900 + integer;
      }
    }

    return JSDate::New(
        ctx,
        detail::TimeClip(
            detail::UTC(detail::MakeDate(detail::MakeDay(y, m, dt),
                                         detail::MakeTime(h, min, s, milli)))));
  } else {
    const double t = detail::TimeClip(detail::CurrentTime() * 1000.0);
    const double dst = detail::DaylightSavingTA(t);
    double offset = detail::LocalTZA() + dst;
    const double time = t + offset;
    char sign = '+';
    if (offset < 0) {
      sign = '-';
      offset = -(detail::LocalTZA() + dst);
    }

    // calc tz info
    int tz_min = offset / detail::kMsPerMinute;
    const int tz_hour = tz_min / 60;
    tz_min %= 60;

    std::tr1::array<char, 50> buf;
    int num = std::snprintf(
        buf.data(), buf.size(),
        "%3s %3s %02d %4d %02d:%02d:%02d GMT%c%02d%02d (%s)",
        detail::WeekDayToString(time),
        detail::MonthToString(time),
        detail::DateFromTime(time),
        detail::YearFromTime(time),
        detail::HourFromTime(time),
        detail::MinFromTime(time),
        detail::SecFromTime(time),
        sign,
        tz_hour,
        tz_min,
        detail::LocalTimeZone(time));
    return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
  }
}

// section 15.9.4.3
// Date.UTC(year, month[, date[, hours[, minutes[, seconds[, ms]]]])
inline JSVal DateUTC(const Arguments& args, Error* error) {
  const std::size_t args_size = args.size();
  Context* const ctx = args.ctx();
  double y;
  if (args_size > 0) {
    y = args[0].ToNumber(ctx, ERROR(error));
  } else {
    y = JSValData::kNaN;
  }

  double m;
  if (args_size > 1) {
    m = args[1].ToNumber(ctx, ERROR(error));
  } else {
    m = JSValData::kNaN;
  }

  double dt;
  if (args_size > 2) {
    dt = args[2].ToNumber(ctx, ERROR(error));
  } else {
    dt = 1;
  }

  double h;
  if (args_size > 3) {
    h = args[3].ToNumber(ctx, ERROR(error));
  } else {
    h = 0;
  }

  double min;
  if (args_size > 4) {
    min = args[4].ToNumber(ctx, ERROR(error));
  } else {
    min = 0;
  }

  double s;
  if (args_size > 5) {
    s = args[5].ToNumber(ctx, ERROR(error));
  } else {
    s = 0;
  }

  double milli;
  if (args_size > 6) {
    milli = args[6].ToNumber(ctx, ERROR(error));
  } else {
    milli = 0;
  }

  if (!std::isnan(y)) {
    const double integer = core::DoubleToInteger(y);
    if (0 <= integer && integer <= 99) {
      y = 1900 + integer;
    }
  }

  return detail::TimeClip(
      detail::MakeDate(detail::MakeDay(y, m, dt),
                       detail::MakeTime(h, min, s, milli)));
}

// section 15.9.4.4 Date.now()
inline JSVal DateNow(const Arguments& args, Error* error) {
  return std::floor(detail::CurrentTime() * 1000.0);
}

// section 15.9.5.2 Date.prototype.toString()
inline JSVal DateToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.toString", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double t = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(t)) {
      return JSString::NewAsciiString(args.ctx(), "Invalid Date");
    } else {
      const double dst = detail::DaylightSavingTA(t);
      double offset = detail::LocalTZA() + dst;
      const double time = t + offset;
      char sign = '+';
      if (offset < 0) {
        sign = '-';
        offset = -(detail::LocalTZA() + dst);
      }

      // calc tz info
      int tz_min = offset / detail::kMsPerMinute;
      const int tz_hour = tz_min / 60;
      tz_min %= 60;

      std::tr1::array<char, 50> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%3s %3s %02d %4d %02d:%02d:%02d GMT%c%02d%02d (%s)",
          detail::WeekDayToString(time),
          detail::MonthToString(time),
          detail::DateFromTime(time),
          detail::YearFromTime(time),
          detail::HourFromTime(time),
          detail::MinFromTime(time),
          detail::SecFromTime(time),
          sign,
          tz_hour,
          tz_min,
          detail::LocalTimeZone(time));
      return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toString is not generic function");
  return JSUndefined;
}

// section 15.9.5.3 Date.prototype.toDateString()
inline JSVal DateToDateString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.toDateString", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double t = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(t)) {
      return JSString::NewAsciiString(args.ctx(), "Invalid Date");
    } else {
      const double time = detail::LocalTime(t);
      std::tr1::array<char, 20> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%3s %3s %02d %4d",
          detail::WeekDayToString(time),
          detail::MonthToString(time),
          detail::DateFromTime(time),
          detail::YearFromTime(time));
      return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toDateString is not generic function");
  return JSUndefined;
}

// section 15.9.5.4 Date.prototype.toTimeString()
inline JSVal DateToTimeString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.toTimeString", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double t = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(t)) {
      return JSString::NewAsciiString(args.ctx(), "Invalid Date");
    } else {
      const double dst = detail::DaylightSavingTA(t);
      double offset = detail::LocalTZA() + dst;
      const double time = t + offset;
      char sign = '+';
      if (offset < 0) {
        sign = '-';
        offset = -(detail::LocalTZA() + dst);
      }

      // calc tz info
      int tz_min = offset / detail::kMsPerMinute;
      const int tz_hour = tz_min / 60;
      tz_min %= 60;

      std::tr1::array<char, 40> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%02d:%02d:%02d GMT%c%02d%02d (%s)",
          detail::HourFromTime(time),
          detail::MinFromTime(time),
          detail::SecFromTime(time),
          sign,
          tz_hour,
          tz_min,
          detail::LocalTimeZone(time));
      return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toTimeString is not generic function");
  return JSUndefined;
}

// section 15.9.5.5 Date.prototype.toLocaleString()
inline JSVal DateToLocaleString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.toLocaleString", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double t = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(t)) {
      return JSString::NewAsciiString(args.ctx(), "Invalid Date");
    } else {
      const double dst = detail::DaylightSavingTA(t);
      double offset = detail::LocalTZA() + dst;
      const double time = t + offset;
      char sign = '+';
      if (offset < 0) {
        sign = '-';
        offset = -(detail::LocalTZA() + dst);
      }

      // calc tz info
      int tz_min = offset / detail::kMsPerMinute;
      const int tz_hour = tz_min / 60;
      tz_min %= 60;

      std::tr1::array<char, 50> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%3s %3s %02d %4d %02d:%02d:%02d GMT%c%02d%02d (%s)",
          detail::WeekDayToString(time),
          detail::MonthToString(time),
          detail::DateFromTime(time),
          detail::YearFromTime(time),
          detail::HourFromTime(time),
          detail::MinFromTime(time),
          detail::SecFromTime(time),
          sign,
          tz_hour,
          tz_min,
          detail::LocalTimeZone(time));
      return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toLocaleString is not generic function");
  return JSUndefined;
}

// section 15.9.5.6 Date.prototype.toLocaleDateString()
inline JSVal DateToLocaleDateString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.toLocaleDateString", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double t = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(t)) {
      return JSString::NewAsciiString(args.ctx(), "Invalid Date");
    } else {
      const double time = detail::LocalTime(t);
      std::tr1::array<char, 20> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%3s %3s %02d %4d",
          detail::WeekDayToString(time),
          detail::MonthToString(time),
          detail::DateFromTime(time),
          detail::YearFromTime(time));
      return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toLocaleDateString is not generic function");
  return JSUndefined;
}

// section 15.9.5.7 Date.prototype.toLocaleTimeString()
inline JSVal DateToLocaleTimeString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.toLocaleTimeString", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double t = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(t)) {
      return JSString::NewAsciiString(args.ctx(), "Invalid Date");
    } else {
      const double dst = detail::DaylightSavingTA(t);
      double offset = detail::LocalTZA() + dst;
      const double time = t + offset;
      char sign = '+';
      if (offset < 0) {
        sign = '-';
        offset = -(detail::LocalTZA() + dst);
      }

      // calc tz info
      int tz_min = offset / detail::kMsPerMinute;
      const int tz_hour = tz_min / 60;
      tz_min %= 60;

      std::tr1::array<char, 40> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%02d:%02d:%02d GMT%c%02d%02d (%s)",
          detail::HourFromTime(time),
          detail::MinFromTime(time),
          detail::SecFromTime(time),
          sign,
          tz_hour,
          tz_min,
          detail::LocalTimeZone(time));
      return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toLocaleTimeString is not generic function");
  return JSUndefined;
}

// section 15.9.5.8 Date.prototype.valueOf()
inline JSVal DateValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.valueOf", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    return static_cast<JSDate*>(obj.object())->value();
  }
  error->Report(Error::Type,
                "Date.prototype.valueOf is not generic function");
  return JSUndefined;
}

// section 15.9.5.9 Date.prototype.getTime()
inline JSVal DateGetTime(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getTime", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    return static_cast<JSDate*>(obj.object())->value();
  }
  error->Report(Error::Type,
                "Date.prototype.getTime is not generic function");
  return JSUndefined;
}

// section 15.9.5.10 Date.prototype.getFullYear()
inline JSVal DateGetFullYear(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getFullYear", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::YearFromTime(detail::LocalTime(time));
  }
  error->Report(Error::Type,
                "Date.prototype.getFullYear is not generic function");
  return JSUndefined;
}

// section 15.9.5.11 Date.prototype.getUTCFullYear()
inline JSVal DateGetUTCFullYear(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCFullYear", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::YearFromTime(time);
  }
  error->Report(Error::Type,
                "Date.prototype.getUTCFullYear is not generic function");
  return JSUndefined;
}

// section 15.9.5.12 Date.prototype.getMonth()
inline JSVal DateGetMonth(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getMonth", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::MonthFromTime(detail::LocalTime(time));
  }
  error->Report(Error::Type,
                "Date.prototype.getMonth is not generic function");
  return JSUndefined;
}

// section 15.9.5.13 Date.prototype.getUTCMonth()
inline JSVal DateGetUTCMonth(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCMonth", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::MonthFromTime(time);
  }
  error->Report(Error::Type,
                "Date.prototype.getUTCMonth is not generic function");
  return JSUndefined;
}

// section 15.9.5.14 Date.prototype.getDate()
inline JSVal DateGetDate(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getDate", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::DateFromTime(detail::LocalTime(time));
  }
  error->Report(Error::Type,
                "Date.prototype.getDate is not generic function");
  return JSUndefined;
}

// section 15.9.5.15 Date.prototype.getUTCDate()
inline JSVal DateGetUTCDate(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCDate", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::DateFromTime(time);
  }
  error->Report(Error::Type,
                "Date.prototype.getUTCDate is not generic function");
  return JSUndefined;
}

// section 15.9.5.16 Date.prototype.getDay()
inline JSVal DateGetDay(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getDay", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::WeekDay(detail::LocalTime(time));
  }
  error->Report(Error::Type,
                "Date.prototype.getDay is not generic function");
  return JSUndefined;
}

// section 15.9.5.17 Date.prototype.getUTCDay()
inline JSVal DateGetUTCDay(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCDay", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::WeekDay(time);
  }
  error->Report(Error::Type,
                "Date.prototype.getUTCDay is not generic function");
  return JSUndefined;
}

// section 15.9.5.18 Date.prototype.getHours()
inline JSVal DateGetHours(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getHours", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::HourFromTime(detail::LocalTime(time));
  }
  error->Report(Error::Type,
                "Date.prototype.getHours is not generic function");
  return JSUndefined;
}

// section 15.9.5.19 Date.prototype.getUTCHours()
inline JSVal DateGetUTCHours(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCHours", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::HourFromTime(time);
  }
  error->Report(Error::Type,
                "Date.prototype.getUTCHours is not generic function");
  return JSUndefined;
}

// section 15.9.5.20 Date.prototype.getMinutes()
inline JSVal DateGetMinutes(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getMinutes", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::MinFromTime(detail::LocalTime(time));
  }
  error->Report(Error::Type,
                "Date.prototype.getMinutes is not generic function");
  return JSUndefined;
}

// section 15.9.5.21 Date.prototype.getUTCMinutes()
inline JSVal DateGetUTCMinutes(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCMinutes", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::MinFromTime(time);
  }
  error->Report(Error::Type,
                "Date.prototype.getUTCMinutes is not generic function");
  return JSUndefined;
}

// section 15.9.5.22 Date.prototype.getSeconds()
inline JSVal DateGetSeconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getSeconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::SecFromTime(detail::LocalTime(time));
  }
  error->Report(Error::Type,
                "Date.prototype.getSeconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.23 Date.prototype.getUTCSeconds()
inline JSVal DateGetUTCSeconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCSeconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::SecFromTime(time);
  }
  error->Report(Error::Type,
                "Date.prototype.getUTCSeconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.24 Date.prototype.getMilliseconds()
inline JSVal DateGetMilliseconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getMilliseconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::MsFromTime(detail::LocalTime(time));
  }
  error->Report(Error::Type,
                "Date.prototype.getMilliseconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.25 Date.prototype.getUTCMilliseconds()
inline JSVal DateGetUTCMilliseconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCMilliseconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return detail::MsFromTime(time);
  }
  error->Report(Error::Type,
                "Date.prototype.getUTCMilliseconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.26 Date.prototype.getTimezoneOffset()
inline JSVal DateGetTimezoneOffset(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getTimezoneOffset", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return (time - detail::LocalTime(time)) / detail::kMsPerMinute;
  }
  error->Report(Error::Type,
                "Date.prototype.getTimezoneOffset is not generic function");
  return JSUndefined;
}

// section 15.9.5.27 Date.prototype.setTime(time)
inline JSVal DateSetTime(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setTime", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    double v;
    if (args.size() > 0) {
      const double val = args[0].ToNumber(args.ctx(), ERROR(error));
      v = detail::TimeClip(val);
    } else {
      v = JSValData::kNaN;
    }
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setTime is not generic function");
  return JSUndefined;
}

// section 15.9.5.28 Date.prototype.setMilliseconds(ms)
inline JSVal DateSetMilliseconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setMilliseconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = detail::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    double ms;
    if (args.size() > 0) {
      ms = args[0].ToNumber(args.ctx(), ERROR(error));
    } else {
      ms = JSValData::kNaN;
    }
    const double v = detail::TimeClip(
        detail::UTC(
            detail::MakeDate(
                detail::Day(t),
                detail::MakeTime(detail::HourFromTime(t),
                                 detail::MinFromTime(t),
                                 detail::SecFromTime(t),
                                 ms))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setMilliseconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.29 Date.prototype.setUTCMilliseconds(ms)
inline JSVal DateSetUTCMilliseconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setUTCMilliseconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    double ms;
    if (args.size() > 0) {
      ms = args[0].ToNumber(args.ctx(), ERROR(error));
    } else {
      ms = JSValData::kNaN;
    }
    const double v = detail::TimeClip(
        detail::MakeDate(
            detail::Day(t),
            detail::MakeTime(detail::HourFromTime(t),
                             detail::MinFromTime(t),
                             detail::SecFromTime(t),
                             ms)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setUTCMilliseconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.30 Date.prototype.setSeconds(sec[, ms])
inline JSVal DateSetSeconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setSeconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = detail::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double sec;
    double ms;
    if (args_size > 0) {
      sec = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        ms = args[1].ToNumber(args.ctx(), ERROR(error));
      } else {
        ms = detail::MsFromTime(t);
      }
    } else {
      sec = JSValData::kNaN;
      ms = detail::MsFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::UTC(
            detail::MakeDate(
                detail::Day(t),
                detail::MakeTime(detail::HourFromTime(t),
                                 detail::MinFromTime(t),
                                 sec,
                                 ms))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setSeconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.31 Date.prototype.setUTCSeconds(sec[, ms])
inline JSVal DateSetUTCSeconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setUTCSeconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    const std::size_t args_size = args.size();
    double sec;
    double ms;
    if (args_size > 0) {
      sec = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        ms = args[1].ToNumber(args.ctx(), ERROR(error));
      } else {
        ms = detail::MsFromTime(t);
      }
    } else {
      sec = JSValData::kNaN;
      ms = detail::MsFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::MakeDate(
            detail::Day(t),
            detail::MakeTime(detail::HourFromTime(t),
                             detail::MinFromTime(t),
                             sec,
                             ms)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setUTCSeconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.32 Date.prototype.setMinutes(min[, sec[, ms]])
inline JSVal DateSetMinutes(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setMinutes", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = detail::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double m;
    double sec;
    double ms;
    if (args_size > 0) {
      m = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        sec = args[1].ToNumber(args.ctx(), ERROR(error));
        if (args_size > 2) {
          ms = args[2].ToNumber(args.ctx(), ERROR(error));
        } else {
          ms = detail::MsFromTime(t);
        }
      } else {
        sec = detail::SecFromTime(t);
        ms = detail::MsFromTime(t);
      }
    } else {
      m = JSValData::kNaN;
      sec = detail::SecFromTime(t);
      ms = detail::MsFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::UTC(
            detail::MakeDate(
                detail::Day(t),
                detail::MakeTime(detail::HourFromTime(t),
                                 m,
                                 sec,
                                 ms))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setMinutes is not generic function");
  return JSUndefined;
}

// section 15.9.5.33 Date.prototype.setUTCMinutes(min[, sec[, ms]])
inline JSVal DateSetUTCMinutes(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setUTCMinutes", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    const std::size_t args_size = args.size();
    double m;
    double sec;
    double ms;
    if (args_size > 0) {
      m = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        sec = args[1].ToNumber(args.ctx(), ERROR(error));
        if (args_size > 2) {
          ms = args[2].ToNumber(args.ctx(), ERROR(error));
        } else {
          ms = detail::MsFromTime(t);
        }
      } else {
        sec = detail::SecFromTime(t);
        ms = detail::MsFromTime(t);
      }
    } else {
      m = JSValData::kNaN;
      sec = detail::SecFromTime(t);
      ms = detail::MsFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::MakeDate(
            detail::Day(t),
            detail::MakeTime(detail::HourFromTime(t),
                             m,
                             sec,
                             ms)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setUTCMinutes is not generic function");
  return JSUndefined;
}

// section 15.9.5.34 Date.prototype.setHours(hour[, min[, sec[, ms]])
inline JSVal DateSetHours(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setHours", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = detail::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double h;
    double m;
    double sec;
    double ms;
    if (args_size > 0) {
      h = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        m = args[1].ToNumber(args.ctx(), ERROR(error));
        if (args_size > 2) {
          sec = args[2].ToNumber(args.ctx(), ERROR(error));
          if (args_size > 3) {
            ms = args[3].ToNumber(args.ctx(), ERROR(error));
          } else {
            ms = detail::MsFromTime(t);
          }
        } else {
          sec = detail::MsFromTime(t);
          ms = detail::MsFromTime(t);
        }
      } else {
        m = detail::MinFromTime(t);
        sec = detail::SecFromTime(t);
        ms = detail::MsFromTime(t);
      }
    } else {
      h = JSValData::kNaN;
      m = detail::MinFromTime(t);
      sec = detail::SecFromTime(t);
      ms = detail::MsFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::UTC(
            detail::MakeDate(
                detail::Day(t),
                detail::MakeTime(h, m, sec, ms))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setHours is not generic function");
  return JSUndefined;
}

// section 15.9.5.35 Date.prototype.setUTCHours(hour[, min[, sec[, ms]])
inline JSVal DateSetUTCHours(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setUTCHours", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    const std::size_t args_size = args.size();
    double h;
    double m;
    double sec;
    double ms;
    if (args_size > 0) {
      h = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        m = args[1].ToNumber(args.ctx(), ERROR(error));
        if (args_size > 2) {
          sec = args[2].ToNumber(args.ctx(), ERROR(error));
          if (args_size > 3) {
            ms = args[3].ToNumber(args.ctx(), ERROR(error));
          } else {
            ms = detail::MsFromTime(t);
          }
        } else {
          sec = detail::MsFromTime(t);
          ms = detail::MsFromTime(t);
        }
      } else {
        m = detail::MinFromTime(t);
        sec = detail::SecFromTime(t);
        ms = detail::MsFromTime(t);
      }
    } else {
      h = JSValData::kNaN;
      m = detail::MinFromTime(t);
      sec = detail::SecFromTime(t);
      ms = detail::MsFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::MakeDate(
            detail::Day(t),
            detail::MakeTime(h, m, sec, ms)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setUTCHours is not generic function");
  return JSUndefined;
}

// section 15.9.5.36 Date.prototype.setDate(date)
inline JSVal DateSetDate(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setDate", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = detail::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double dt;
    if (args_size > 0) {
      dt = args[0].ToNumber(args.ctx(), ERROR(error));
    } else {
      dt = JSValData::kNaN;
    }
    const double v = detail::TimeClip(
        detail::UTC(
            detail::MakeDate(
                detail::MakeDay(detail::YearFromTime(t),
                                detail::MonthFromTime(t),
                                dt),
                detail::TimeWithinDay(t))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setDate is not generic function");
  return JSUndefined;
}

// section 15.9.5.37 Date.prototype.setUTCDate(date)
inline JSVal DateSetUTCDate(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setUTCDate", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    const std::size_t args_size = args.size();
    double dt;
    if (args_size > 0) {
      dt = args[0].ToNumber(args.ctx(), ERROR(error));
    } else {
      dt = JSValData::kNaN;
    }
    const double v = detail::TimeClip(
        detail::MakeDate(
            detail::MakeDay(detail::YearFromTime(t),
                            detail::MonthFromTime(t),
                            dt),
            detail::TimeWithinDay(t)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setUTCDate is not generic function");
  return JSUndefined;
}

// section 15.9.5.38 Date.prototype.setMonth(month[, date])
inline JSVal DateSetMonth(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setMonth", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = detail::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double m;
    double dt;
    if (args_size > 0) {
      m = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        dt = args[1].ToNumber(args.ctx(), ERROR(error));
      } else {
        dt = detail::DateFromTime(t);
      }
    } else {
      m = JSValData::kNaN;
      dt = detail::DateFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::UTC(
            detail::MakeDate(
                detail::MakeDay(detail::YearFromTime(t),
                                m,
                                dt),
                detail::TimeWithinDay(t))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setMonth is not generic function");
  return JSUndefined;
}

// section 15.9.5.39 Date.prototype.setUTCMonth(month[, date])
inline JSVal DateSetUTCMonth(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setUTCMonth", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    const std::size_t args_size = args.size();
    double m;
    double dt;
    if (args_size > 0) {
      m = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        dt = args[1].ToNumber(args.ctx(), ERROR(error));
      } else {
        dt = detail::DateFromTime(t);
      }
    } else {
      m = JSValData::kNaN;
      dt = detail::DateFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::MakeDate(
            detail::MakeDay(detail::YearFromTime(t),
                            m,
                            dt),
            detail::TimeWithinDay(t)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setUTCMonth is not generic function");
  return JSUndefined;
}

// section 15.9.5.40 Date.prototype.setFullYear(year[, month[, date]])
inline JSVal DateSetFullYear(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setFullYear", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    double t = detail::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    if (std::isnan(t)) {
      t = +0.0;
    }
    const std::size_t args_size = args.size();
    double y;
    double m;
    double dt;
    if (args_size > 0) {
      y = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        m = args[1].ToNumber(args.ctx(), ERROR(error));
        if (args_size > 2) {
          dt = args[2].ToNumber(args.ctx(), ERROR(error));
        } else {
          dt = detail::DateFromTime(t);
        }
      } else {
        m = detail::MonthFromTime(t);
        dt = detail::DateFromTime(t);
      }
    } else {
      y = JSValData::kNaN;
      m = detail::MonthFromTime(t);
      dt = detail::DateFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::UTC(
            detail::MakeDate(
                detail::MakeDay(y,
                                m,
                                dt),
                detail::TimeWithinDay(t))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setFullYear is not generic function");
  return JSUndefined;
}

// section 15.9.5.41 Date.prototype.setUTCFullYear(year[, month[, date]])
inline JSVal DateSetUTCFullYear(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setUTCFullYear", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    double t = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(t)) {
      t = +0.0;
    }
    const std::size_t args_size = args.size();
    double y;
    double m;
    double dt;
    if (args_size > 0) {
      y = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        m = args[1].ToNumber(args.ctx(), ERROR(error));
        if (args_size > 2) {
          dt = args[2].ToNumber(args.ctx(), ERROR(error));
        } else {
          dt = detail::DateFromTime(t);
        }
      } else {
        m = detail::MonthFromTime(t);
        dt = detail::DateFromTime(t);
      }
    } else {
      y = JSValData::kNaN;
      m = detail::MonthFromTime(t);
      dt = detail::DateFromTime(t);
    }
    const double v = detail::TimeClip(
        detail::MakeDate(
            detail::MakeDay(y,
                            m,
                            dt),
            detail::TimeWithinDay(t)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  error->Report(Error::Type,
                "Date.prototype.setUTCFullYear is not generic function");
  return JSUndefined;
}

// section 15.9.5.42 Date.prototype.toUTCString()
inline JSVal DateToUTCString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.toUTCString", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSString::NewAsciiString(args.ctx(), "Invalid Date");
    } else {
      std::tr1::array<char, 32> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%3s, %02d %3s %4d %02d:%02d:%02d GMT",
          detail::WeekDayToString(time),
          detail::DateFromTime(time),
          detail::MonthToString(time),
          detail::YearFromTime(time),
          detail::HourFromTime(time),
          detail::MinFromTime(time),
          detail::SecFromTime(time));
      return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toUTCString is not generic function");
  return JSUndefined;
}

// section 15.9.5.43 Date.prototype.toISOString()
inline JSVal DateToISOString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.toISOString", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSString::NewAsciiString(args.ctx(), "Invalid Date");
    } else {
      std::tr1::array<char, 32> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%4d-%02d-%02dT%02d:%02d:%02d.%03dZ",
          detail::YearFromTime(time),
          detail::MonthFromTime(time)+1,
          detail::DateFromTime(time),
          detail::HourFromTime(time),
          detail::MinFromTime(time),
          detail::SecFromTime(time),
          detail::MsFromTime(time));
      return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toISOString is not generic function");
  return JSUndefined;
}

// section 15.9.5.44 Date.prototype.toJSON()
inline JSVal DateToJSON(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.toJSON", args, error);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
  const JSVal tv = JSVal(obj).ToPrimitive(ctx, Hint::NUMBER, ERROR(error));

  if (tv.IsNumber()) {
    const double& val = tv.number();
    if (!std::isfinite(val)) {
      return JSNull;
    }
  }

  const JSVal toISO = obj->Get(
      ctx,
      ctx->Intern("toISOString"), ERROR(error));

  if (!toISO.IsCallable()) {
    error->Report(Error::Type, "toISOString is not function");
    return JSUndefined;
  }
  return toISO.object()->AsCallable()->Call(Arguments(ctx, obj), ERROR(error));
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_STRING_H_
