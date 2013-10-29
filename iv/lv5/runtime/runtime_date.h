#ifndef IV_LV5_RUNTIME_DATE_H_
#define IV_LV5_RUNTIME_DATE_H_
#include <ctime>
#include <cmath>
#include <cstring>
#include <iv/detail/cstdint.h>
#include <iv/detail/array.h>
#include <iv/utils.h>
#include <iv/platform_math.h>
#include <iv/conversions.h>
#include <iv/canonicalized_nan.h>
#include <iv/date_utils.h>
#include <iv/date_parser.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/jsdate.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsval.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.9.2.1
//   Date([year[, month[, date[, hours[, minutes[, seconds[, ms]]]])
// section 15.9.3.1
//   new Date(year, month[, date[, hours[, minutes[, seconds[, ms]]]])
inline JSVal DateConstructor(const Arguments& args, Error* e) {
  if (args.IsConstructorCalled()) {
    const std::size_t args_size = args.size();
    Context* const ctx = args.ctx();

    if (args_size == 0) {
      // section 15.9.3.3 new Date()
      return JSDate::New(ctx, core::date::TimeClip(core::date::CurrentTime()));
    }

    if (args_size == 1) {
      // section 15.9.3.2 new Date(value)
      const JSVal v = args[0].ToPrimitive(ctx, Hint::NONE, IV_LV5_ERROR(e));
      if (v.IsString()) {
        JSString* str = v.string();
        if (str->Is8Bit()) {
          return JSDate::New(
              ctx,
              core::date::TimeClip(core::date::Parse(*str->Get8Bit())));
        } else {
          return JSDate::New(
              ctx,
              core::date::TimeClip(core::date::Parse(*str->Get16Bit())));
        }
      } else {
        const double V = v.ToNumber(ctx, IV_LV5_ERROR(e));
        return JSDate::New(ctx, core::date::TimeClip(V));
      }
    }

    // following case, args_size > 1
    double y = args[0].ToNumber(ctx, IV_LV5_ERROR(e));
    const double m = args[1].ToNumber(ctx, IV_LV5_ERROR(e));

    double dt;
    if (args_size > 2) {
      dt = args[2].ToNumber(ctx, IV_LV5_ERROR(e));
    } else {
      dt = 1;
    }

    double h;
    if (args_size > 3) {
      h = args[3].ToNumber(ctx, IV_LV5_ERROR(e));
    } else {
      h = 0;
    }

    double min;
    if (args_size > 4) {
      min = args[4].ToNumber(ctx, IV_LV5_ERROR(e));
    } else {
      min = 0;
    }

    double s;
    if (args_size > 5) {
      s = args[5].ToNumber(ctx, IV_LV5_ERROR(e));
    } else {
      s = 0;
    }

    double milli;
    if (args_size > 6) {
      milli = args[6].ToNumber(ctx, IV_LV5_ERROR(e));
    } else {
      milli = 0;
    }

    if (!core::math::IsNaN(y)) {
      const double integer = core::DoubleToInteger(y);
      if (0 <= integer && integer <= 99) {
        y = 1900 + integer;
      }
    }

    return JSDate::New(
        ctx,
        core::date::TimeClip(
            core::date::UTC(
                core::date::MakeDate(
                    core::date::MakeDay(y, m, dt),
                    core::date::MakeTime(h, min, s, milli)))));
  }

  const double utc = core::date::TimeClip(core::date::CurrentTime());
  const double local = core::date::LocalTime(utc);
  const int timezone = -((utc - local) / core::date::kMsPerMinute);

  const char sign = (timezone < 0) ? '-' : '+';
  const int offset = (timezone < 0) ? -timezone : timezone;
  const int tz_min = offset % 60;
  const int tz_hour = offset / 60;

  char buf[40];
  const core::date::DateInstance instance(local);
  const int num = snprintf(
      buf, sizeof(buf) - 1,
      "%3s %3s %02d %4d %02d:%02d:%02d GMT%c%02d%02d",
      instance.WeekDayString(),
      instance.MonthString(),
      instance.date(),
      instance.year(),
      instance.hour(),
      instance.min(),
      instance.sec(),
      sign,
      tz_hour,
      tz_min);
  return JSString::NewAsciiString(
      args.ctx(), core::StringPiece(buf, num), e);
}


// section 15.9.4.2 Date.parse(string)
inline JSVal DateParse(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.parse", args, e);
  const JSVal first = (args.empty()) ? JSUndefined : args[0];
  const JSString* target = first.ToString(args.ctx(), IV_LV5_ERROR(e));
  if (target->Is8Bit()) {
    return core::date::Parse(*target->Get8Bit());
  } else {
    return core::date::Parse(*target->Get16Bit());
  }
}

// section 15.9.4.3
// Date.UTC(year, month[, date[, hours[, minutes[, seconds[, ms]]]])
inline JSVal DateUTC(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.UTC", args, e);
  const std::size_t args_size = args.size();
  Context* const ctx = args.ctx();
  double y;
  if (args_size > 0) {
    y = args[0].ToNumber(ctx, IV_LV5_ERROR(e));
  } else {
    y = core::kNaN;
  }

  double m;
  if (args_size > 1) {
    m = args[1].ToNumber(ctx, IV_LV5_ERROR(e));
  } else {
    m = core::kNaN;
  }

  double dt;
  if (args_size > 2) {
    dt = args[2].ToNumber(ctx, IV_LV5_ERROR(e));
  } else {
    dt = 1;
  }

  double h;
  if (args_size > 3) {
    h = args[3].ToNumber(ctx, IV_LV5_ERROR(e));
  } else {
    h = 0;
  }

  double min;
  if (args_size > 4) {
    min = args[4].ToNumber(ctx, IV_LV5_ERROR(e));
  } else {
    min = 0;
  }

  double s;
  if (args_size > 5) {
    s = args[5].ToNumber(ctx, IV_LV5_ERROR(e));
  } else {
    s = 0;
  }

  double milli;
  if (args_size > 6) {
    milli = args[6].ToNumber(ctx, IV_LV5_ERROR(e));
  } else {
    milli = 0;
  }

  if (!core::math::IsNaN(y)) {
    const double integer = core::DoubleToInteger(y);
    if (0 <= integer && integer <= 99) {
      y = 1900 + integer;
    }
  }

  return core::date::TimeClip(
      core::date::MakeDate(
          core::date::MakeDay(y, m, dt),
          core::date::MakeTime(h, min, s, milli)));
}

// section 15.9.4.4 Date.now()
inline JSVal DateNow(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.now", args, e);
  return std::floor(core::date::CurrentTime());
}

// section 15.9.5.2 Date.prototype.toString()
inline JSVal DateToString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.toString", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.toString is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return JSString::NewAsciiString(args.ctx(), "Invalid Date", e);
  }

  const int timezone = -date->timezone();
  const char sign = (timezone < 0) ? '-' : '+';
  const int offset = (timezone < 0) ? -timezone : timezone;
  const int tz_min = offset % 60;
  const int tz_hour = offset / 60;

  char buf[40];
  const core::date::DateInstance& instance = date->local();
  const int num = snprintf(
      buf, sizeof(buf) - 1,
      "%3s %3s %02d %4d %02d:%02d:%02d GMT%c%02d%02d",
      instance.WeekDayString(),
      instance.MonthString(),
      instance.date(),
      instance.year(),
      instance.hour(),
      instance.min(),
      instance.sec(),
      sign,
      tz_hour,
      tz_min);
  return JSString::NewAsciiString(
      args.ctx(), core::StringPiece(buf, num), e);
}

// section 15.9.5.3 Date.prototype.toDateString()
inline JSVal DateToDateString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.toDateString", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.toDateString is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    e->Report(Error::Range, "Invalid Date");
    return JSEmpty;
  }

  char buf[20];
  const core::date::DateInstance& instance = date->local();
  const int num = snprintf(
      buf, sizeof(buf) - 1,
      "%3s %3s %02d %4d",
      instance.WeekDayString(),
      instance.MonthString(),
      instance.date(),
      instance.year());
  return JSString::NewAsciiString(
      args.ctx(), core::StringPiece(buf, num), e);
}

// section 15.9.5.4 Date.prototype.toTimeString()
inline JSVal DateToTimeString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.toTimeString", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.toTimeString is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    e->Report(Error::Range, "Invalid Date");
    return JSEmpty;
  }

  const int timezone = -date->timezone();
  const char sign = (timezone < 0) ? '-' : '+';
  const int offset = (timezone < 0) ? -timezone : timezone;
  const int tz_min = offset % 60;
  const int tz_hour = offset / 60;

  char buf[20];
  const core::date::DateInstance& instance = date->local();
  const int num = snprintf(
      buf, sizeof(buf) - 1,
      "%02d:%02d:%02d GMT%c%02d%02d",
      instance.hour(),
      instance.min(),
      instance.sec(),
      sign,
      tz_hour,
      tz_min);
  return JSString::NewAsciiString(
      args.ctx(), core::StringPiece(buf, num), e);
}

// section 15.9.5.5 Date.prototype.toLocaleString()
inline JSVal DateToLocaleString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.toLocaleString", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.toLocaleString is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return JSString::NewAsciiString(args.ctx(), "Invalid Date", e);
  }

  const int timezone = -date->timezone();
  const char sign = (timezone < 0) ? '-' : '+';
  const int offset = (timezone < 0) ? -timezone : timezone;
  const int tz_min = offset % 60;
  const int tz_hour = offset / 60;

  char buf[40];
  const core::date::DateInstance& instance = date->local();
  const int num = snprintf(
      buf, sizeof(buf) - 1,
      "%3s %3s %02d %4d %02d:%02d:%02d GMT%c%02d%02d",
      instance.WeekDayString(),
      instance.MonthString(),
      instance.date(),
      instance.year(),
      instance.hour(),
      instance.min(),
      instance.sec(),
      sign,
      tz_hour,
      tz_min);
  return JSString::NewAsciiString(
      args.ctx(), core::StringPiece(buf, num), e);
}

// section 15.9.5.6 Date.prototype.toLocaleDateString()
inline JSVal DateToLocaleDateString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.toLocaleDateString", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.toLocaleDateString is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    e->Report(Error::Range, "Invalid Date");
    return JSEmpty;
  }

  const core::date::DateInstance& instance = date->local();
  char buf[20];
  const int num = snprintf(
      buf, sizeof(buf) - 1,
      "%3s %3s %02d %4d",
      instance.WeekDayString(),
      instance.MonthString(),
      instance.date(),
      instance.year());
  return JSString::NewAsciiString(
      args.ctx(), core::StringPiece(buf, num), e);
}

// section 15.9.5.7 Date.prototype.toLocaleTimeString()
inline JSVal DateToLocaleTimeString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.toLocaleTimeString", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.toLocaleTimeString is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    e->Report(Error::Range, "Invalid Date");
    return JSEmpty;
  }

  const int timezone = -date->timezone();
  const char sign = (timezone < 0) ? '-' : '+';
  const int offset = (timezone < 0) ? -timezone : timezone;
  const int tz_min = offset % 60;
  const int tz_hour = offset / 60;

  char buf[20];
  const core::date::DateInstance& instance = date->local();
  const int num = snprintf(
      buf, sizeof(buf) - 1,
      "%02d:%02d:%02d GMT%c%02d%02d",
      instance.hour(),
      instance.min(),
      instance.sec(),
      sign,
      tz_hour,
      tz_min);
  return JSString::NewAsciiString(
      args.ctx(), core::StringPiece(buf, num), e);
}

// section 15.9.5.8 Date.prototype.valueOf()
inline JSVal DateValueOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.valueOf", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.valueOf is not generic function");
    return JSUndefined;
  }

  // this is date object
  return static_cast<JSDate*>(obj.object())->value();
}

// section 15.9.5.9 Date.prototype.getTime()
inline JSVal DateGetTime(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getTime", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getTime is not generic function");
    return JSUndefined;
  }

  // this is date object
  return static_cast<JSDate*>(obj.object())->value();
}

// section 15.9.5.10 Date.prototype.getFullYear()
inline JSVal DateGetFullYear(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getFullYear", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getFullYear is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->local().year());
}

// section 15.9.5.11 Date.prototype.getUTCFullYear()
inline JSVal DateGetUTCFullYear(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getUTCFullYear", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getUTCFullYear is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->utc().year());
}

// section 15.9.5.12 Date.prototype.getMonth()
inline JSVal DateGetMonth(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getMonth", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getMonth is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->local().month());
}

// section 15.9.5.13 Date.prototype.getUTCMonth()
inline JSVal DateGetUTCMonth(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getUTCMonth", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getUTCMonth is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->utc().month());
}

// section 15.9.5.14 Date.prototype.getDate()
inline JSVal DateGetDate(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getDate", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getDate is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->local().date());
}

// section 15.9.5.15 Date.prototype.getUTCDate()
inline JSVal DateGetUTCDate(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getUTCDate", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getUTCDate is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->utc().date());
}

// section 15.9.5.16 Date.prototype.getDay()
inline JSVal DateGetDay(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getDay", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getDay is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->local().weekday());
}

// section 15.9.5.17 Date.prototype.getUTCDay()
inline JSVal DateGetUTCDay(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getUTCDay", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getUTCDay is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->utc().weekday());
}

// section 15.9.5.18 Date.prototype.getHours()
inline JSVal DateGetHours(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getHours", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getHours is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->local().hour());
}

// section 15.9.5.19 Date.prototype.getUTCHours()
inline JSVal DateGetUTCHours(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getUTCHours", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getUTCHours is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->utc().hour());
}

// section 15.9.5.20 Date.prototype.getMinutes()
inline JSVal DateGetMinutes(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getMinutes", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getMinutes is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->local().min());
}

// section 15.9.5.21 Date.prototype.getUTCMinutes()
inline JSVal DateGetUTCMinutes(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getUTCMinutes", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getUTCMinutes is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->utc().min());
}

// section 15.9.5.22 Date.prototype.getSeconds()
inline JSVal DateGetSeconds(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getSeconds", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getSeconds is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->local().sec());
}

// section 15.9.5.23 Date.prototype.getUTCSeconds()
inline JSVal DateGetUTCSeconds(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getUTCSeconds", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getUTCSeconds is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->utc().sec());
}

// section 15.9.5.24 Date.prototype.getMilliseconds()
inline JSVal DateGetMilliseconds(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getMilliseconds", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getMilliseconds is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->local().ms());
}

// section 15.9.5.25 Date.prototype.getUTCMilliseconds()
inline JSVal DateGetUTCMilliseconds(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getUTCMilliseconds", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getUTCMilliseconds is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    return core::kNaN;
  }
  return JSVal::Int32(date->utc().ms());
}

// section 15.9.5.26 Date.prototype.getTimezoneOffset()
inline JSVal DateGetTimezoneOffset(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getTimezoneOffset", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getTimezoneOffset is not generic function");
    return JSUndefined;
  }

  // this is date object
  return static_cast<JSDate*>(obj.object())->timezone();
}

// section 15.9.5.27 Date.prototype.setTime(time)
inline JSVal DateSetTime(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setTime", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.setTime is not generic function");
    return JSUndefined;
  }

  // this is date object
  double v = core::kNaN;
  if (!args.empty()) {
    const double val = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    v = core::date::TimeClip(val);
  }
  static_cast<JSDate*>(obj.object())->set_value(v);
  return v;
}

// section 15.9.5.28 Date.prototype.setMilliseconds(ms)
inline JSVal DateSetMilliseconds(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setMilliseconds", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = core::date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    double ms = core::kNaN;
    if (!args.empty()) {
      ms = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    }
    const double v = core::date::TimeClip(
        core::date::UTC(
            core::date::MakeDate(
                core::date::Day(t),
                core::date::MakeTime(
                    core::date::HourFromTime(t),
                    core::date::MinFromTime(t),
                    core::date::SecFromTime(t),
                    ms))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setMilliseconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.29 Date.prototype.setUTCMilliseconds(ms)
inline JSVal DateSetUTCMilliseconds(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setUTCMilliseconds", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    double ms = core::kNaN;
    if (!args.empty()) {
      ms = args.front().ToNumber(args.ctx(), IV_LV5_ERROR(e));
    }
    const double v = core::date::TimeClip(
        core::date::MakeDate(
            core::date::Day(t),
            core::date::MakeTime(
                core::date::HourFromTime(t),
                core::date::MinFromTime(t),
                core::date::SecFromTime(t),
                ms)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setUTCMilliseconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.30 Date.prototype.setSeconds(sec[, ms])
inline JSVal DateSetSeconds(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setSeconds", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = core::date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double sec;
    double ms;
    if (args_size > 0) {
      sec = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 1) {
        ms = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      } else {
        ms = core::date::MsFromTime(t);
      }
    } else {
      sec = core::kNaN;
      ms = core::date::MsFromTime(t);
    }
    const double v = core::date::TimeClip(
        core::date::UTC(
            core::date::MakeDate(
                core::date::Day(t),
                core::date::MakeTime(
                    core::date::HourFromTime(t),
                    core::date::MinFromTime(t),
                    sec,
                    ms))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setSeconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.31 Date.prototype.setUTCSeconds(sec[, ms])
inline JSVal DateSetUTCSeconds(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setUTCSeconds", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    const std::size_t args_size = args.size();
    double sec;
    double ms;
    if (args_size > 0) {
      sec = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 1) {
        ms = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      } else {
        ms = core::date::MsFromTime(t);
      }
    } else {
      sec = core::kNaN;
      ms = core::date::MsFromTime(t);
    }
    const double v = core::date::TimeClip(
        core::date::MakeDate(
            core::date::Day(t),
            core::date::MakeTime(
                core::date::HourFromTime(t),
                core::date::MinFromTime(t),
                sec,
                ms)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setUTCSeconds is not generic function");
  return JSUndefined;
}

// section 15.9.5.32 Date.prototype.setMinutes(min[, sec[, ms]])
inline JSVal DateSetMinutes(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setMinutes", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = core::date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double m;
    double sec;
    double ms;
    if (args_size > 0) {
      m = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 1) {
        sec = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
        if (args_size > 2) {
          ms = args[2].ToNumber(args.ctx(), IV_LV5_ERROR(e));
        } else {
          ms = core::date::MsFromTime(t);
        }
      } else {
        sec = core::date::SecFromTime(t);
        ms = core::date::MsFromTime(t);
      }
    } else {
      m = core::kNaN;
      sec = core::date::SecFromTime(t);
      ms = core::date::MsFromTime(t);
    }
    const double v = core::date::TimeClip(
        core::date::UTC(
            core::date::MakeDate(
                core::date::Day(t),
                core::date::MakeTime(
                    core::date::HourFromTime(t),
                    m,
                    sec,
                    ms))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setMinutes is not generic function");
  return JSUndefined;
}

// section 15.9.5.33 Date.prototype.setUTCMinutes(min[, sec[, ms]])
inline JSVal DateSetUTCMinutes(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setUTCMinutes", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    const std::size_t args_size = args.size();
    double m;
    double sec;
    double ms;
    if (args_size > 0) {
      m = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 1) {
        sec = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
        if (args_size > 2) {
          ms = args[2].ToNumber(args.ctx(), IV_LV5_ERROR(e));
        } else {
          ms = core::date::MsFromTime(t);
        }
      } else {
        sec = core::date::SecFromTime(t);
        ms = core::date::MsFromTime(t);
      }
    } else {
      m = core::kNaN;
      sec = core::date::SecFromTime(t);
      ms = core::date::MsFromTime(t);
    }
    const double v = core::date::TimeClip(
        core::date::MakeDate(
            core::date::Day(t),
            core::date::MakeTime(
                core::date::HourFromTime(t),
                m,
                sec,
                ms)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setUTCMinutes is not generic function");
  return JSUndefined;
}

// section 15.9.5.34 Date.prototype.setHours(hour[, min[, sec[, ms]])
inline JSVal DateSetHours(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setHours", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = core::date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double h;
    double m;
    double sec;
    double ms;
    if (args_size > 0) {
      h = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 1) {
        m = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
        if (args_size > 2) {
          sec = args[2].ToNumber(args.ctx(), IV_LV5_ERROR(e));
          if (args_size > 3) {
            ms = args[3].ToNumber(args.ctx(), IV_LV5_ERROR(e));
          } else {
            ms = core::date::MsFromTime(t);
          }
        } else {
          sec = core::date::MsFromTime(t);
          ms = core::date::MsFromTime(t);
        }
      } else {
        m = core::date::MinFromTime(t);
        sec = core::date::SecFromTime(t);
        ms = core::date::MsFromTime(t);
      }
    } else {
      h = core::kNaN;
      m = core::date::MinFromTime(t);
      sec = core::date::SecFromTime(t);
      ms = core::date::MsFromTime(t);
    }
    const double v = core::date::TimeClip(
        core::date::UTC(
            core::date::MakeDate(
                core::date::Day(t),
                core::date::MakeTime(h, m, sec, ms))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setHours is not generic function");
  return JSUndefined;
}

// section 15.9.5.35 Date.prototype.setUTCHours(hour[, min[, sec[, ms]])
inline JSVal DateSetUTCHours(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setUTCHours", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    const std::size_t args_size = args.size();
    double h;
    double m;
    double sec;
    double ms;
    if (args_size > 0) {
      h = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 1) {
        m = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
        if (args_size > 2) {
          sec = args[2].ToNumber(args.ctx(), IV_LV5_ERROR(e));
          if (args_size > 3) {
            ms = args[3].ToNumber(args.ctx(), IV_LV5_ERROR(e));
          } else {
            ms = core::date::MsFromTime(t);
          }
        } else {
          sec = core::date::MsFromTime(t);
          ms = core::date::MsFromTime(t);
        }
      } else {
        m = core::date::MinFromTime(t);
        sec = core::date::SecFromTime(t);
        ms = core::date::MsFromTime(t);
      }
    } else {
      h = core::kNaN;
      m = core::date::MinFromTime(t);
      sec = core::date::SecFromTime(t);
      ms = core::date::MsFromTime(t);
    }
    const double v = core::date::TimeClip(
        core::date::MakeDate(
            core::date::Day(t),
            core::date::MakeTime(h, m, sec, ms)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setUTCHours is not generic function");
  return JSUndefined;
}

// section 15.9.5.36 Date.prototype.setDate(date)
inline JSVal DateSetDate(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setDate", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = core::date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    double dt = core::kNaN;
    if (!args.empty()) {
      dt = args.front().ToNumber(args.ctx(), IV_LV5_ERROR(e));
    }
    const double v = core::date::TimeClip(
        core::date::UTC(
            core::date::MakeDate(
                core::date::MakeDay(
                    core::date::YearFromTime(t),
                    core::date::MonthFromTime(t),
                    dt),
                core::date::TimeWithinDay(t))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setDate is not generic function");
  return JSUndefined;
}

// section 15.9.5.37 Date.prototype.setUTCDate(date)
inline JSVal DateSetUTCDate(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setUTCDate", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    double dt = core::kNaN;
    if (!args.empty()) {
      dt = args.front().ToNumber(args.ctx(), IV_LV5_ERROR(e));
    }
    const double v = core::date::TimeClip(
        core::date::MakeDate(
            core::date::MakeDay(
                core::date::YearFromTime(t),
                core::date::MonthFromTime(t),
                dt),
            core::date::TimeWithinDay(t)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setUTCDate is not generic function");
  return JSUndefined;
}

// section 15.9.5.38 Date.prototype.setMonth(month[, date])
inline JSVal DateSetMonth(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setMonth", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = core::date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    const std::size_t args_size = args.size();
    double m;
    double dt;
    if (args_size > 0) {
      m = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 1) {
        dt = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      } else {
        dt = core::date::DateFromTime(t);
      }
    } else {
      m = core::kNaN;
      dt = core::date::DateFromTime(t);
    }
    const double v = core::date::TimeClip(
        core::date::UTC(
            core::date::MakeDate(
                core::date::MakeDay(
                    core::date::YearFromTime(t),
                    m,
                    dt),
                core::date::TimeWithinDay(t))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setMonth is not generic function");
  return JSUndefined;
}

// section 15.9.5.39 Date.prototype.setUTCMonth(month[, date])
inline JSVal DateSetUTCMonth(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setUTCMonth", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    const double t = static_cast<JSDate*>(obj.object())->value();
    const std::size_t args_size = args.size();
    double m;
    double dt;
    if (args_size > 0) {
      m = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 1) {
        dt = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      } else {
        dt = core::date::DateFromTime(t);
      }
    } else {
      m = core::kNaN;
      dt = core::date::DateFromTime(t);
    }
    const double v = core::date::TimeClip(
        core::date::MakeDate(
            core::date::MakeDay(
                core::date::YearFromTime(t),
                m,
                dt),
            core::date::TimeWithinDay(t)));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setUTCMonth is not generic function");
  return JSUndefined;
}

// section 15.9.5.40 Date.prototype.setFullYear(year[, month[, date]])
inline JSVal DateSetFullYear(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setFullYear", args, e);
  const JSVal obj = args.this_binding();
  if (obj.IsObject() && obj.object()->IsClass<Class::Date>()) {
    double t = core::date::LocalTime(
        static_cast<JSDate*>(obj.object())->value());
    if (core::math::IsNaN(t)) {
      t = +0.0;
    }
    const std::size_t args_size = args.size();
    double y;
    double m;
    double dt;
    if (args_size > 0) {
      y = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 1) {
        m = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
        if (args_size > 2) {
          dt = args[2].ToNumber(args.ctx(), IV_LV5_ERROR(e));
        } else {
          dt = core::date::DateFromTime(t);
        }
      } else {
        m = core::date::MonthFromTime(t);
        dt = core::date::DateFromTime(t);
      }
    } else {
      y = core::kNaN;
      m = core::date::MonthFromTime(t);
      dt = core::date::DateFromTime(t);
    }
    const double v = core::date::TimeClip(
        core::date::UTC(
            core::date::MakeDate(
                core::date::MakeDay(y, m, dt),
                core::date::TimeWithinDay(t))));
    static_cast<JSDate*>(obj.object())->set_value(v);
    return v;
  }
  e->Report(Error::Type,
            "Date.prototype.setFullYear is not generic function");
  return JSUndefined;
}

// section 15.9.5.41 Date.prototype.setUTCFullYear(year[, month[, date]])
inline JSVal DateSetUTCFullYear(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setUTCFullYear", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.setUTCFullYear is not generic function");
    return JSUndefined;
  }

  double t = static_cast<JSDate*>(obj.object())->value();
  if (core::math::IsNaN(t)) {
    t = +0.0;
  }
  const std::size_t args_size = args.size();
  double y;
  double m;
  double dt;
  if (args_size > 0) {
    y = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(e));
    if (args_size > 1) {
      m = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      if (args_size > 2) {
        dt = args[2].ToNumber(args.ctx(), IV_LV5_ERROR(e));
      } else {
        dt = core::date::DateFromTime(t);
      }
    } else {
      m = core::date::MonthFromTime(t);
      dt = core::date::DateFromTime(t);
    }
  } else {
    y = core::kNaN;
    m = core::date::MonthFromTime(t);
    dt = core::date::DateFromTime(t);
  }
  const double v = core::date::TimeClip(
      core::date::MakeDate(
          core::date::MakeDay(y, m, dt),
          core::date::TimeWithinDay(t)));
  static_cast<JSDate*>(obj.object())->set_value(v);
  return v;
}

// section 15.9.5.42 Date.prototype.toUTCString()
inline JSVal DateToUTCString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.toUTCString", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.toUTCString is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    e->Report(Error::Range, "Invalid Date");
    return JSEmpty;
  }

  char buf[40];
  const core::date::DateInstance& instance = date->utc();
  const int num = snprintf(
      buf, sizeof(buf) - 1,
      "%3s, %02d %3s %4d %02d:%02d:%02d GMT",
      instance.WeekDayString(),
      instance.date(),
      instance.MonthString(),
      instance.year(),
      instance.hour(),
      instance.min(),
      instance.sec());
  return JSString::NewAsciiString(
      args.ctx(), core::StringPiece(buf, num), e);
}

// section 15.9.5.43 Date.prototype.toISOString()
inline JSVal DateToISOString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.toISOString", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.toISOString is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->utc().IsValid()) {
    e->Report(Error::Range, "Invalid Date");
    return JSEmpty;
  }

  char buf[30];
  const core::date::DateInstance& instance = date->utc();
  const int num = snprintf(
      buf, sizeof(buf) - 1,
      "%4d-%02d-%02dT%02d:%02d:%02d.%03dZ",
      instance.year(),
      instance.month() + 1,
      instance.date(),
      instance.hour(),
      instance.min(),
      instance.sec(),
      instance.ms());
  return JSString::NewAsciiString(
      args.ctx(), core::StringPiece(buf, num), e);
}

// section 15.9.5.44 Date.prototype.toJSON()
inline JSVal DateToJSON(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.toJSON", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  const JSVal tv = JSVal(obj).ToPrimitive(ctx, Hint::NUMBER, IV_LV5_ERROR(e));

  if (tv.IsNumber()) {
    const double& val = tv.number();
    if (!core::math::IsFinite(val)) {
      return JSNull;
    }
  }

  const JSVal toISO =
      obj->Get(ctx, ctx->Intern("toISOString"), IV_LV5_ERROR(e));

  if (!toISO.IsCallable()) {
    e->Report(Error::Type, "toISOString is not function");
    return JSUndefined;
  }
  ScopedArguments a(ctx, 0, IV_LV5_ERROR(e));
  return static_cast<JSFunction*>(toISO.object())->Call(&a, obj, e);
}

// section B.2.4 Date.prototype.getYear()
// this method is deprecated.
inline JSVal DateGetYear(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.getYear", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.getYear is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* date = static_cast<JSDate*>(obj.object());
  if (!date->local().IsValid()) {
    return core::kNaN;
  }
  return date->local().year() - 1900;
}

// section B.2.5 Date.prototype.setYear(year)
// this method is deprecated.
inline JSVal DateSetYear(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Date.prototype.setYear", args, e);
  const JSVal obj = args.this_binding();
  if (!obj.IsObject() || !obj.object()->IsClass<Class::Date>()) {
    e->Report(Error::Type,
              "Date.prototype.setYear is not generic function");
    return JSUndefined;
  }

  // this is date object
  JSDate* d = static_cast<JSDate*>(obj.object());
  double t = core::date::LocalTime(d->value());
  if (core::math::IsNaN(t)) {
    t = +0.0;
  }
  double y = core::kNaN;
  if (!args.empty()) {
    y = args.front().ToNumber(args.ctx(), IV_LV5_ERROR(e));
  }
  if (core::math::IsNaN(y)) {
    d->set_value(y);
    return y;
  }
  double year = core::DoubleToInteger(y);
  if (0.0 <= year && year <= 99.0) {
    year += 1900;
  }
  const double v = core::date::TimeClip(
      core::date::UTC(
          core::date::MakeDate(
              core::date::MakeDay(year,
                                  core::date::MonthFromTime(t),
                                  core::date::DateFromTime(t)),
              core::date::TimeWithinDay(t))));
  d->set_value(v);
  return v;
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_STRING_H_
