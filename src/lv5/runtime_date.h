#ifndef _IV_LV5_RUNTIME_DATE_H_
#define _IV_LV5_RUNTIME_DATE_H_
#include <ctime>
#include <cmath>
#include <cstring>
#include <tr1/cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "utils.h"
#include "conversions.h"
#include "jsdate.h"
#include "jsstring.h"
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
  return 365 * (y - 1970) +
      ((y - 1969) / 4) -
      ((y - 1901) / 100) +
      ((y - 1601) / 400);
}

inline double TimeFromYear(int y) {
  return kMsPerDay * DaysFromYear(y);
}

// JSC's method
inline int YearFromTime(double t) {
  const int about = core::DoubleToInt32(
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

inline int32_t LocalTZA() {
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

// TODO(Constellation) implement it
inline double DaylightSavingTA(double t) {
  return 0.0;
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
  const int monthday = MonthToDaysInYear(month, IsLeapYear(year));
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
  if (!std::isfinite(day) || !std::isfinite(time)) {
    return JSValData::kNaN;
  } else {
    return day * kMsPerDay + time;
  }
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

#ifdef _WIN32
inline double CurrentTime() {
  FILETIME ft;
  LARGE_INTEGER i;
  GetSystemTimeAsFileTime(&ft);
  i.LowPart = ft.dwLowDateTime;
  i.HighPart = ft.dwHighDateTime;
  return (i.QuadPart - kEpochTime) / 10 / 1000000;
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
//   Date([year[, month[, date[, hours[, minutes[, seconds[,ms ]]]])
// section 15.9.3.1
//   new Date(year, month[, date[, hours[, minutes[, seconds[,ms ]]]])
inline JSVal DateConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    const std::size_t args_size = args.size();
    Context* const ctx = args.ctx();

    if (args_size == 0) {
      // section 15.9.3.3 new Date()
      return JSDate::New(
          ctx, detail::CurrentTime() * 1000.0);
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
    // TODO(Constellation)
    return JSUndefined;
  }
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
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSString::NewAsciiString(args.ctx(), "Invalid Date");
    } else {
      std::string buffer;
      buffer.append(detail::WeekDayToString(time));
      buffer.push_back(' ');
      buffer.append(detail::MonthToString(time));
      // buffer.push_back(' ');
      // buffer.append(detail::DateToString(time));
      // buffer.push_back(' ');
      // buffer.append(detail::YearToString(time));
      return JSString::New(args.ctx(), buffer);
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toString is not generic function");
  return JSUndefined;
}

// section 15.9.5.8 Date.prototype.valueOf()
inline JSVal DateValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.valueOf", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
    // this is date object
    return static_cast<JSDate*>(obj.object())->value();
  }
  error->Report(Error::Type,
                "Date.prototype.getTime is not generic function");
  return JSUndefined;
}

// section 15.9.5.11 Date.prototype.getUTCFullYear()
inline JSVal DateGetUTCFullYear(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCFullYear", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.13 Date.prototype.getUTCMonth()
inline JSVal DateGetUTCMonth(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCMonth", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.15 Date.prototype.getUTCDate()
inline JSVal DateGetUTCDate(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCDate", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.17 Date.prototype.getUTCDay()
inline JSVal DateGetUTCDay(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCDay", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.19 Date.prototype.getUTCHours()
inline JSVal DateGetUTCHours(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCHours", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.21 Date.prototype.getUTCMinutes()
inline JSVal DateGetUTCMinutes(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCMinutes", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.23 Date.prototype.getUTCSeconds()
inline JSVal DateGetUTCSeconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCSeconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.25 Date.prototype.getUTCMilliseconds()
inline JSVal DateGetUTCMilliseconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.getUTCMilliseconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.27 Date.prototype.setTime(time)
inline JSVal DateSetTime(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setTime", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.29 Date.prototype.setUTCMilliseconds(ms)
inline JSVal DateSetUTCMilliseconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setUTCMilliseconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

// section 15.9.5.31 Date.prototype.setUTCSeconds(sec[, ms])
inline JSVal DateSetUTCSeconds(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.prototype.setUTCSeconds", args, error);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->cls()) {
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

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_STRING_H_
