#ifndef IV_LV5_RUNTIME_I18N_H_
#define IV_LV5_RUNTIME_I18N_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsi18n.h>
#include <iv/lv5/runtime_array.h>
namespace iv {
namespace lv5 {
namespace runtime {


inline JSVal LocaleListConstructor(const Arguments& args, Error* e) {
  return JSLocaleList::CreateLocaleList(args.ctx(), args.At(0), e);
}

#ifdef IV_ENABLE_I18N
inline JSVal CollatorConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  JSCollator* obj = JSCollator::New(ctx);
  return JSCollator::Initialize(ctx, obj, args.At(0), args.At(1), e);
}

inline JSVal CollatorCompareGetter(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.Collator.compare", args, e);
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->IsClass<Class::Collator>()) {
    e->Report(Error::Type,
              "Intl.Collator.prototype.compare is not generic getter");
    return JSEmpty;
  }
  JSCollator* obj = static_cast<JSCollator*>(o);
  JSVal bound = obj->GetField(JSCollator::BOUND_COMPARE);
  if (bound.IsUndefined()) {
    bound = JSCollatorBoundFunction::New(ctx, obj);
    obj->SetField(JSCollator::BOUND_COMPARE, bound);
  }
  return bound;
}

inline JSVal CollatorResolvedOptionsGetter(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.Collator.resolvedOptions", args, e);
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->IsClass<Class::Collator>()) {
    e->Report(Error::Type,
              "Intl.Collator.prototype.resolvedOptions is not generic getter");
    return JSEmpty;
  }
  JSCollator* collator = static_cast<JSCollator*>(o);
  JSObject* obj = JSObject::New(ctx);
  bind::Object(ctx, obj)
      .def(context::Intern(ctx, "collation"),
           collator->GetField(JSCollator::COLLATION),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "usage"),
           collator->GetField(JSCollator::USAGE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "sensitivity"),
           collator->GetField(JSCollator::SENSITIVITY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "numeric"),
           collator->GetField(JSCollator::NUMERIC),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "normalization"),
           collator->GetField(JSCollator::NORMALIZATION),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "caseFirst"),
           collator->GetField(JSCollator::CASE_FIRST),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "ignorePunctuation"),
           collator->GetField(JSCollator::IGNORE_PUNCTUATION))
      .def(context::Intern(ctx, "locale"),
           collator->GetField(JSCollator::LOCALE),
           ATTR::W | ATTR::E | ATTR::C);
  return obj;
}

inline JSVal CollatorSupportedLocalesOf(const Arguments& args, Error* e) {
  return detail_i18n::SupportedLocales(
      args.ctx(),
      ICUStringIteration(icu::Collator::getAvailableLocales()),
      ICUStringIteration(),
      args.At(0), args.At(1), e);
}

inline JSVal NumberFormatConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  JSNumberFormat* obj = JSNumberFormat::New(ctx);
  return JSNumberFormat::Initialize(ctx, obj, args.At(0), args.At(1), e);
}

inline JSVal NumberFormatFormat(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.NumberFormat.prototype.format", args, e);
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->IsClass<Class::NumberFormat>()) {
    e->Report(Error::Type,
              "Intl.NumberFormat.prototype.format is not generic function");
    return JSEmpty;
  }
  JSNumberFormat* format = static_cast<JSNumberFormat*>(o);
  const double value = args.At(0).ToNumber(ctx, IV_LV5_ERROR(e));
  return format->Format(ctx, value, e);
}

inline JSVal NumberFormatResolvedOptionsGetter(
    const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.NumberFormat.resolvedOptions", args, e);
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->IsClass<Class::NumberFormat>()) {
    e->Report(
        Error::Type,
        "Intl.NumberFormat.prototype.resolvedOptions is not generic getter");
    return JSEmpty;
  }
  JSNumberFormat* format = static_cast<JSNumberFormat*>(o);
  JSObject* obj = JSObject::New(ctx);
  bind::Object(ctx, obj)
      .def(context::Intern(ctx, "style"),
           format->GetField(JSNumberFormat::STYLE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "currency"),
           format->GetField(JSNumberFormat::CURRENCY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "currencyDisplay"),
           format->GetField(JSNumberFormat::CURRENCY_DISPLAY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "minimumIntegerDigits"),
           format->GetField(JSNumberFormat::MINIMUM_INTEGER_DIGITS),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "minimumFractionDigits"),
           format->GetField(JSNumberFormat::MINIMUM_FRACTION_DIGITS),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "maximumFractionDigits"),
           format->GetField(JSNumberFormat::MAXIMUM_FRACTION_DIGITS),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "minimumSignificantDigits"),
           format->GetField(JSNumberFormat::MINIMUM_SIGNIFICANT_DIGITS),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "maximumSignificantDigits"),
           format->GetField(JSNumberFormat::MAXIMUM_SIGNIFICANT_DIGITS),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "useGrouping"),
           format->GetField(JSNumberFormat::USE_GROUPING))
      .def(context::Intern(ctx, "locale"),
           format->GetField(JSNumberFormat::LOCALE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "numberingSystem"),
           format->GetField(JSNumberFormat::NUMBERING_SYSTEM),
           ATTR::W | ATTR::E | ATTR::C);
  return obj;
}

inline JSVal NumberFormatSupportedLocalesOf(const Arguments& args, Error* e) {
  return detail_i18n::SupportedLocales(
      args.ctx(),
      ICUStringIteration(icu::NumberFormat::getAvailableLocales()),
      ICUStringIteration(),
      args.At(0), args.At(1), e);
}

inline JSVal DateTimeFormatConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  JSDateTimeFormat* obj = JSDateTimeFormat::New(ctx);
  return JSDateTimeFormat::Initialize(ctx, obj, args.At(0), args.At(1), e);
}

inline JSVal DateTimeFormatFormat(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.DateTimeFormat.prototype.format", args, e);
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->IsClass<Class::DateTimeFormat>()) {
    e->Report(Error::Type,
              "Intl.DateTimeFormat.prototype.format is not generic function");
    return JSEmpty;
  }
  JSDateTimeFormat* format = static_cast<JSDateTimeFormat*>(o);
  const JSVal arg1 = args.At(0);
  const double value =
      (arg1.IsUndefined()) ?
        std::floor(core::date::CurrentTime()) :
        args.At(0).ToNumber(ctx, IV_LV5_ERROR(e));
  return format->Format(ctx, value, e);
}

inline JSVal DateTimeFormatResolvedOptionsGetter(
    const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.DateTimeFormat.resolvedOptions", args, e);
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->IsClass<Class::DateTimeFormat>()) {
    e->Report(
        Error::Type,
        "Intl.DateTimeFormat.prototype.resolvedOptions is not generic getter");
    return JSEmpty;
  }
  JSDateTimeFormat* format = static_cast<JSDateTimeFormat*>(o);
  JSObject* obj = JSObject::New(ctx);
  bind::Object(ctx, obj)
      .def(context::Intern(ctx, "calendar"),
           format->GetField(JSDateTimeFormat::CALENDAR),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "timeZone"),
           format->GetField(JSDateTimeFormat::TIME_ZONE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "hour12"),
           format->GetField(JSDateTimeFormat::HOUR12),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "locale"),
           format->GetField(JSDateTimeFormat::LOCALE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "numberingSystem"),
           format->GetField(JSDateTimeFormat::NUMBERING_SYSTEM),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "weekday"),
           format->GetField(JSDateTimeFormat::WEEKDAY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "era"),
           format->GetField(JSDateTimeFormat::ERA),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "year"),
           format->GetField(JSDateTimeFormat::YEAR))
      .def(context::Intern(ctx, "month"),
           format->GetField(JSDateTimeFormat::MONTH),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "day"),
           format->GetField(JSDateTimeFormat::DAY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "hour"),
           format->GetField(JSDateTimeFormat::HOUR),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "minute"),
           format->GetField(JSDateTimeFormat::MINUTE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "second"),
           format->GetField(JSDateTimeFormat::SECOND),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "timeZoneName"),
           format->GetField(JSDateTimeFormat::TIME_ZONE_NAME),
           ATTR::W | ATTR::E | ATTR::C);
  return obj;
}

inline JSVal DateTimeFormatSupportedLocalesOf(const Arguments& args, Error* e) {
  int32_t num = 0;
  const icu::Locale* avail = icu::DateFormat::getAvailableLocales(num);
  std::vector<std::string> vec(num);
  for (int32_t i = 0; i < num; ++i) {
    vec[i] = avail[i].getName();
  }
  return detail_i18n::SupportedLocales(
      args.ctx(),
      vec.begin(),
      vec.end(),
      args.At(0), args.At(1), e);
}
#endif  // IV_ENABLE_I18N_H_

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_I18N_H_
