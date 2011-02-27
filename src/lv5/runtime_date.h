#ifndef _IV_LV5_RUNTIME_DATE_H_
#define _IV_LV5_RUNTIME_DATE_H_
#include <ctime>
#include <cmath>
#include <cstring>
#include <tr1/cstdint>
#include <tr1/array>
#include "utils.h"
#include "conversions.h"
#include "lv5/lv5.h"
#include "lv5/jsdate.h"
#include "lv5/jsstring.h"
#include "lv5/jsval.h"
#include "lv5/date_utils.h"
#include "lv5/date_parser.h"
namespace iv {
namespace lv5 {
namespace runtime {

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
          ctx, date::TimeClip(date::CurrentTime() * 1000.0));
    }

    if (args_size == 1) {
      // section 15.9.3.2 new Date(value)
      const JSVal v = args[0].ToPrimitive(ctx, Hint::NONE, ERROR(error));
      if (v.IsString()) {
        return JSDate::New(ctx,
                           date::TimeClip(date::Parse(*v.string())));
      } else {
        const double V = v.ToNumber(ctx, ERROR(error));
        return JSDate::New(ctx, date::TimeClip(V));
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
        date::TimeClip(
            date::UTC(date::MakeDate(date::MakeDay(y, m, dt),
                                     date::MakeTime(h, min, s, milli)))));
  } else {
    const double t = date::TimeClip(date::CurrentTime() * 1000.0);
    const double dst = date::DaylightSavingTA(t);
    double offset = date::LocalTZA() + dst;
    const double time = t + offset;
    char sign = '+';
    if (offset < 0) {
      sign = '-';
      offset = -(date::LocalTZA() + dst);
    }

    // calc tz info
    int tz_min = offset / date::kMsPerMinute;
    const int tz_hour = tz_min / 60;
    tz_min %= 60;

    std::tr1::array<char, 50> buf;
    int num = std::snprintf(
        buf.data(), buf.size(),
        "%3s %3s %02d %4d %02d:%02d:%02d GMT%c%02d%02d (%s)",
        date::WeekDayToString(time),
        date::MonthToString(time),
        date::DateFromTime(time),
        date::YearFromTime(time),
        date::HourFromTime(time),
        date::MinFromTime(time),
        date::SecFromTime(time),
        sign,
        tz_hour,
        tz_min,
        date::LocalTimeZone(time));
    return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
  }
}


// section 15.9.4.2 Date.parse(string)
inline JSVal DateParse(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Date.parse", args, e);
  const JSVal first = (args.size() == 0) ? JSUndefined : args[0];
  const JSString* target = first.ToString(args.ctx(), ERROR(e));
  return date::Parse(*target);
}

// section 15.9.4.3
// Date.UTC(year, month[, date[, hours[, minutes[, seconds[, ms]]]])
inline JSVal DateUTC(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.UTC", args, error);
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

  return date::TimeClip(
      date::MakeDate(date::MakeDay(y, m, dt),
                     date::MakeTime(h, min, s, milli)));
}

// section 15.9.4.4 Date.now()
inline JSVal DateNow(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Date.now", args, error);
  return std::floor(date::CurrentTime() * 1000.0);
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
      const double dst = date::DaylightSavingTA(t);
      double offset = date::LocalTZA() + dst;
      const double time = t + offset;
      char sign = '+';
      if (offset < 0) {
        sign = '-';
        offset = -(date::LocalTZA() + dst);
      }

      // calc tz info
      int tz_min = offset / date::kMsPerMinute;
      const int tz_hour = tz_min / 60;
      tz_min %= 60;

      std::tr1::array<char, 50> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%3s %3s %02d %4d %02d:%02d:%02d GMT%c%02d%02d (%s)",
          date::WeekDayToString(time),
          date::MonthToString(time),
          date::DateFromTime(time),
          date::YearFromTime(time),
          date::HourFromTime(time),
          date::MinFromTime(time),
          date::SecFromTime(time),
          sign,
          tz_hour,
          tz_min,
          date::LocalTimeZone(time));
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
      const double time = date::LocalTime(t);
      std::tr1::array<char, 20> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%3s %3s %02d %4d",
          date::WeekDayToString(time),
          date::MonthToString(time),
          date::DateFromTime(time),
          date::YearFromTime(time));
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
      const double dst = date::DaylightSavingTA(t);
      double offset = date::LocalTZA() + dst;
      const double time = t + offset;
      char sign = '+';
      if (offset < 0) {
        sign = '-';
        offset = -(date::LocalTZA() + dst);
      }

      // calc tz info
      int tz_min = offset / date::kMsPerMinute;
      const int tz_hour = tz_min / 60;
      tz_min %= 60;

      std::tr1::array<char, 40> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%02d:%02d:%02d GMT%c%02d%02d (%s)",
          date::HourFromTime(time),
          date::MinFromTime(time),
          date::SecFromTime(time),
          sign,
          tz_hour,
          tz_min,
          date::LocalTimeZone(time));
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
      const double dst = date::DaylightSavingTA(t);
      double offset = date::LocalTZA() + dst;
      const double time = t + offset;
      char sign = '+';
      if (offset < 0) {
        sign = '-';
        offset = -(date::LocalTZA() + dst);
      }

      // calc tz info
      int tz_min = offset / date::kMsPerMinute;
      const int tz_hour = tz_min / 60;
      tz_min %= 60;

      std::tr1::array<char, 50> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%3s %3s %02d %4d %02d:%02d:%02d GMT%c%02d%02d (%s)",
          date::WeekDayToString(time),
          date::MonthToString(time),
          date::DateFromTime(time),
          date::YearFromTime(time),
          date::HourFromTime(time),
          date::MinFromTime(time),
          date::SecFromTime(time),
          sign,
          tz_hour,
          tz_min,
          date::LocalTimeZone(time));
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
      const double time = date::LocalTime(t);
      std::tr1::array<char, 20> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%3s %3s %02d %4d",
          date::WeekDayToString(time),
          date::MonthToString(time),
          date::DateFromTime(time),
          date::YearFromTime(time));
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
      const double dst = date::DaylightSavingTA(t);
      double offset = date::LocalTZA() + dst;
      const double time = t + offset;
      char sign = '+';
      if (offset < 0) {
        sign = '-';
        offset = -(date::LocalTZA() + dst);
      }

      // calc tz info
      int tz_min = offset / date::kMsPerMinute;
      const int tz_hour = tz_min / 60;
      tz_min %= 60;

      std::tr1::array<char, 40> buf;
      int num = std::snprintf(
          buf.data(), buf.size(),
          "%02d:%02d:%02d GMT%c%02d%02d (%s)",
          date::HourFromTime(time),
          date::MinFromTime(time),
          date::SecFromTime(time),
          sign,
          tz_hour,
          tz_min,
          date::LocalTimeZone(time));
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
    return date::YearFromTime(date::LocalTime(time));
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
    return date::YearFromTime(time);
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
    return date::MonthFromTime(date::LocalTime(time));
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
    return date::MonthFromTime(time);
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
    return date::DateFromTime(date::LocalTime(time));
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
    return date::DateFromTime(time);
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
    return date::WeekDay(date::LocalTime(time));
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
    return date::WeekDay(time);
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
    return date::HourFromTime(date::LocalTime(time));
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
    return date::HourFromTime(time);
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
    return date::MinFromTime(date::LocalTime(time));
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
    return date::MinFromTime(time);
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
    return date::SecFromTime(date::LocalTime(time));
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
    return date::SecFromTime(time);
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
    return date::MsFromTime(date::LocalTime(time));
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
    return date::MsFromTime(time);
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
    return (time - date::LocalTime(time)) / date::kMsPerMinute;
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
      v = date::TimeClip(val);
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
    const double t = date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    double ms;
    if (args.size() > 0) {
      ms = args[0].ToNumber(args.ctx(), ERROR(error));
    } else {
      ms = JSValData::kNaN;
    }
    const double v = date::TimeClip(
        date::UTC(
            date::MakeDate(
                date::Day(t),
                date::MakeTime(date::HourFromTime(t),
                               date::MinFromTime(t),
                               date::SecFromTime(t),
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
    const double v = date::TimeClip(
        date::MakeDate(
            date::Day(t),
            date::MakeTime(date::HourFromTime(t),
                           date::MinFromTime(t),
                           date::SecFromTime(t),
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
    const double t = date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double sec;
    double ms;
    if (args_size > 0) {
      sec = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        ms = args[1].ToNumber(args.ctx(), ERROR(error));
      } else {
        ms = date::MsFromTime(t);
      }
    } else {
      sec = JSValData::kNaN;
      ms = date::MsFromTime(t);
    }
    const double v = date::TimeClip(
        date::UTC(
            date::MakeDate(
                date::Day(t),
                date::MakeTime(date::HourFromTime(t),
                               date::MinFromTime(t),
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
        ms = date::MsFromTime(t);
      }
    } else {
      sec = JSValData::kNaN;
      ms = date::MsFromTime(t);
    }
    const double v = date::TimeClip(
        date::MakeDate(
            date::Day(t),
            date::MakeTime(date::HourFromTime(t),
                           date::MinFromTime(t),
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
    const double t = date::LocalTime(
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
          ms = date::MsFromTime(t);
        }
      } else {
        sec = date::SecFromTime(t);
        ms = date::MsFromTime(t);
      }
    } else {
      m = JSValData::kNaN;
      sec = date::SecFromTime(t);
      ms = date::MsFromTime(t);
    }
    const double v = date::TimeClip(
        date::UTC(
            date::MakeDate(
                date::Day(t),
                date::MakeTime(date::HourFromTime(t),
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
          ms = date::MsFromTime(t);
        }
      } else {
        sec = date::SecFromTime(t);
        ms = date::MsFromTime(t);
      }
    } else {
      m = JSValData::kNaN;
      sec = date::SecFromTime(t);
      ms = date::MsFromTime(t);
    }
    const double v = date::TimeClip(
        date::MakeDate(
            date::Day(t),
            date::MakeTime(date::HourFromTime(t),
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
    const double t = date::LocalTime(
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
            ms = date::MsFromTime(t);
          }
        } else {
          sec = date::MsFromTime(t);
          ms = date::MsFromTime(t);
        }
      } else {
        m = date::MinFromTime(t);
        sec = date::SecFromTime(t);
        ms = date::MsFromTime(t);
      }
    } else {
      h = JSValData::kNaN;
      m = date::MinFromTime(t);
      sec = date::SecFromTime(t);
      ms = date::MsFromTime(t);
    }
    const double v = date::TimeClip(
        date::UTC(
            date::MakeDate(
                date::Day(t),
                date::MakeTime(h, m, sec, ms))));
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
            ms = date::MsFromTime(t);
          }
        } else {
          sec = date::MsFromTime(t);
          ms = date::MsFromTime(t);
        }
      } else {
        m = date::MinFromTime(t);
        sec = date::SecFromTime(t);
        ms = date::MsFromTime(t);
      }
    } else {
      h = JSValData::kNaN;
      m = date::MinFromTime(t);
      sec = date::SecFromTime(t);
      ms = date::MsFromTime(t);
    }
    const double v = date::TimeClip(
        date::MakeDate(
            date::Day(t),
            date::MakeTime(h, m, sec, ms)));
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
    const double t = date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double dt;
    if (args_size > 0) {
      dt = args[0].ToNumber(args.ctx(), ERROR(error));
    } else {
      dt = JSValData::kNaN;
    }
    const double v = date::TimeClip(
        date::UTC(
            date::MakeDate(
                date::MakeDay(date::YearFromTime(t),
                              date::MonthFromTime(t),
                              dt),
                date::TimeWithinDay(t))));
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
    const double v = date::TimeClip(
        date::MakeDate(
            date::MakeDay(date::YearFromTime(t),
                          date::MonthFromTime(t),
                          dt),
            date::TimeWithinDay(t)));
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
    const double t = date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double m;
    double dt;
    if (args_size > 0) {
      m = args[0].ToNumber(args.ctx(), ERROR(error));
      if (args_size > 1) {
        dt = args[1].ToNumber(args.ctx(), ERROR(error));
      } else {
        dt = date::DateFromTime(t);
      }
    } else {
      m = JSValData::kNaN;
      dt = date::DateFromTime(t);
    }
    const double v = date::TimeClip(
        date::UTC(
            date::MakeDate(
                date::MakeDay(date::YearFromTime(t),
                              m,
                              dt),
                date::TimeWithinDay(t))));
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
        dt = date::DateFromTime(t);
      }
    } else {
      m = JSValData::kNaN;
      dt = date::DateFromTime(t);
    }
    const double v = date::TimeClip(
        date::MakeDate(
            date::MakeDay(date::YearFromTime(t),
                          m,
                          dt),
            date::TimeWithinDay(t)));
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
    double t = date::LocalTime(
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
          dt = date::DateFromTime(t);
        }
      } else {
        m = date::MonthFromTime(t);
        dt = date::DateFromTime(t);
      }
    } else {
      y = JSValData::kNaN;
      m = date::MonthFromTime(t);
      dt = date::DateFromTime(t);
    }
    const double v = date::TimeClip(
        date::UTC(
            date::MakeDate(
                date::MakeDay(y, m, dt),
                date::TimeWithinDay(t))));
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
          dt = date::DateFromTime(t);
        }
      } else {
        m = date::MonthFromTime(t);
        dt = date::DateFromTime(t);
      }
    } else {
      y = JSValData::kNaN;
      m = date::MonthFromTime(t);
      dt = date::DateFromTime(t);
    }
    const double v = date::TimeClip(
        date::MakeDate(
            date::MakeDay(y, m, dt),
            date::TimeWithinDay(t)));
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
          date::WeekDayToString(time),
          date::DateFromTime(time),
          date::MonthToString(time),
          date::YearFromTime(time),
          date::HourFromTime(time),
          date::MinFromTime(time),
          date::SecFromTime(time));
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
          date::YearFromTime(time),
          date::MonthFromTime(time)+1,
          date::DateFromTime(time),
          date::HourFromTime(time),
          date::MinFromTime(time),
          date::SecFromTime(time),
          date::MsFromTime(time));
      return JSString::New(args.ctx(), core::StringPiece(buf.data(), num));
    }
  }
  error->Report(Error::Type,
                "Date.prototype.toISOString is not generic function");
  return JSUndefined;
}

// section 15.9.5.44 Date.prototype.toJSON()
inline JSVal DateToJSON(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Date.prototype.toJSON", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal tv = JSVal(obj).ToPrimitive(ctx, Hint::NUMBER, ERROR(e));

  if (tv.IsNumber()) {
    const double& val = tv.number();
    if (!std::isfinite(val)) {
      return JSNull;
    }
  }

  const JSVal toISO = obj->Get(
      ctx,
      ctx->Intern("toISOString"), ERROR(e));

  if (!toISO.IsCallable()) {
    e->Report(Error::Type, "toISOString is not function");
    return JSUndefined;
  }
  Arguments a(ctx, ERROR(e));
  return toISO.object()->AsCallable()->Call(a, obj, e);
}

// section B.2.4 Date.prototype.getYear()
// this method is deprecated.
inline JSVal DateGetYear(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Date.prototype.getYear", args, e);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    // this is date object
    const double time = static_cast<JSDate*>(obj.object())->value();
    if (std::isnan(time)) {
      return JSValData::kNaN;
    }
    return date::YearFromTime(date::LocalTime(time)) - 1900;
  }
  e->Report(Error::Type,
            "Date.prototype.getYear is not generic function");
  return JSUndefined;
}

// section B.2.5 Date.prototype.setYear(year)
// this method is deprecated.
inline JSVal DateSetYear(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Date.prototype.setYear", args, e);
  const JSVal& obj = args.this_binding();
  const Class& cls = args.ctx()->Cls("Date");
  if (obj.IsObject() && cls.name == obj.object()->class_name()) {
    JSDate* d = static_cast<JSDate*>(obj.object());
    double t = date::LocalTime(d->value());
    if (std::isnan(t)) {
      t = +0.0;
    }
    const std::size_t args_size = args.size();
    double y = JSValData::kNaN;
    if (args_size > 0) {
      y = args[0].ToNumber(args.ctx(), ERROR(e));
    }
    if (std::isnan(y)) {
      d->set_value(y);
      return y;
    }
    double year = core::DoubleToInteger(y);
    if (0.0 <= year && year <= 99.0) {
      year += 1900;
    }
    const double v = date::TimeClip(
        date::UTC(
            date::MakeDate(
                date::MakeDay(year,
                              date::MonthFromTime(t),
                              date::DateFromTime(t)),
                date::TimeWithinDay(t))));
    d->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setYear is not generic function");
  return JSUndefined;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_STRING_H_
