#ifndef IV_LV5_RUNTIME_I18N_H_
#define IV_LV5_RUNTIME_I18N_H_
#ifdef IV_ENABLE_I18N
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
  return lv5::detail_i18n::CreateLocaleList(args.ctx(), args.At(0), e);
}

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
           collator,
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "usage"),
           collator->GetField(JSCollator::USAGE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "sensitivity"),
           collator->GetField(JSCollator::SENSITIVITY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "backwards"),
           collator->GetField(JSCollator::BACKWARDS),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "caseLevel"),
           collator->GetField(JSCollator::CASE_LEVEL),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "numeric"),
           collator->GetField(JSCollator::NUMERIC),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "hiraganaQuaternary"),
           collator->GetField(JSCollator::HIRAGANA_QUATERNARY),
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
  UErrorCode err = U_ZERO_ERROR;
  icu::UnicodeString result;
  static_cast<icu::DecimalFormat*>(
      format->number_format())->format(value, result);
  if (U_FAILURE(err)) {
    e->Report(Error::Type, "Intl.NumberFormat parse failed");
    return JSEmpty;
  }
  return JSString::New(
      ctx, result.getBuffer(), result.getBuffer() + result.length());
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

#undef IV_LV5_I18N_INTL_CHECK
} } }  // namespace iv::lv5::runtime
#endif  // IV_ENABLE_I18N_H_
#endif  // IV_LV5_RUNTIME_I18N_H_
