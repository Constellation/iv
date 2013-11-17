#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsi18n.h>
#include <iv/lv5/context.h>
#include <iv/lv5/runtime/array.h>
#include <iv/lv5/runtime/i18n.h>
namespace iv {
namespace lv5 {
namespace runtime {

#ifdef IV_ENABLE_I18N
JSVal CollatorConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  JSCollator* obj = JSCollator::New(ctx);
  return JSCollator::Initialize(ctx, obj, args.At(0), args.At(1), e);
}

JSVal CollatorCompareGetter(const Arguments& args, Error* e) {
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

JSVal CollatorResolvedOptions(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.Collator.prototype.resolvedOptions", args, e);
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
      .def(ctx->Intern("collation"),
           collator->GetField(JSCollator::COLLATION),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("usage"),
           collator->GetField(JSCollator::USAGE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("sensitivity"),
           collator->GetField(JSCollator::SENSITIVITY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("numeric"),
           collator->GetField(JSCollator::NUMERIC),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("normalization"),
           collator->GetField(JSCollator::NORMALIZATION),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("caseFirst"),
           collator->GetField(JSCollator::CASE_FIRST),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("ignorePunctuation"),
           collator->GetField(JSCollator::IGNORE_PUNCTUATION))
      .def(ctx->Intern("locale"),
           collator->GetField(JSCollator::LOCALE),
           ATTR::W | ATTR::E | ATTR::C);
  return obj;
}

JSVal CollatorSupportedLocalesOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.Collator.supportedLocalesOf", args, e);
  Context* ctx = args.ctx();
  JSVector* requested =
      i18n::CanonicalizeLocaleList(ctx, args.At(0), IV_LV5_ERROR(e));
  return detail_i18n::SupportedLocales(
      args.ctx(),
      ICUStringIteration(icu::Collator::getAvailableLocales()),
      ICUStringIteration(),
      requested, args.At(1), e);
}
#endif  // IV_ENABLE_I18N

JSVal NumberFormatConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  JSObject* obj = nullptr;
  const JSVal t = args.this_binding();
  const JSVal locales = args.At(0);
  const JSVal options = args.At(1);
  if (args.IsConstructorCalled() || t.IsUndefined() || JSVal::SameValue(t, ctx->intl())) {
    obj = JSObject::New(ctx, ctx->global_data()->number_format_map());
  } else {
    obj = t.ToObject(ctx, IV_LV5_ERROR(e));
  }
  if (!obj->IsExtensible()) {
    e->Report(Error::Type, "Intl.NumberFormat to non extensible object");
    return JSEmpty;
  }
  return i18n::InitializeNumberFormat(ctx, obj, locales, options, e);
}

JSVal NumberFormatFormatGetter(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.NumberFormat.format", args, e);
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->HasOwnProperty(ctx, ctx->i18n()->symbols().initializedNumberFormat())) {
    e->Report(Error::Type,
              "Intl.NumberFormat.prototype.format is not generic function");
    return JSEmpty;
  }
  i18n::JSNumberFormatHolder* format =
      i18n::JSNumberFormatHolder::Extract(ctx, o);
  return format->Bound(ctx, e);
}

JSVal NumberFormatFormat(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.NumberFormat.prototype.format", args, e);
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->HasOwnProperty(ctx, ctx->i18n()->symbols().initializedNumberFormat())) {
    e->Report(Error::Type,
              "Intl.NumberFormat.prototype.format is not generic function");
    return JSEmpty;
  }

  i18n::JSNumberFormatHolder* format =
      i18n::JSNumberFormatHolder::Extract(ctx, o);
  const double value = args.At(0).ToNumber(ctx, IV_LV5_ERROR(e));
  return format->Format(ctx, value, e);
}

JSVal NumberFormatResolvedOptions(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.NumberFormat.prototype.resolvedOptions", args, e);
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->HasOwnProperty(ctx, ctx->i18n()->symbols().initializedNumberFormat())) {
    e->Report(
        Error::Type,
        "Intl.NumberFormat.prototype.resolvedOptions is not generic getter");
    return JSEmpty;
  }

  i18n::JSNumberFormatHolder* format =
      i18n::JSNumberFormatHolder::Extract(ctx, o);
  return format->ResolveOptions(ctx, e);
}

JSVal NumberFormatSupportedLocalesOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.NumberFormat.supportedLocalesOf", args, e);
  return i18n::NumberFormatSupportedLocalesOf(args.ctx(),
                                              args.At(0), args.At(1), e);
}

#ifdef IV_ENABLE_I18N

JSVal DateTimeFormatConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  JSDateTimeFormat* obj = JSDateTimeFormat::New(ctx);
  return JSDateTimeFormat::Initialize(ctx, obj, args.At(0), args.At(1), e);
}

JSVal DateTimeFormatFormat(const Arguments& args, Error* e) {
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

JSVal DateTimeFormatResolvedOptions(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.DateTimeFormat.prototype.resolvedOptions", args, e);
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
      .def(ctx->Intern("calendar"),
           format->GetField(JSDateTimeFormat::CALENDAR),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("timeZone"),
           format->GetField(JSDateTimeFormat::TIME_ZONE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("hour12"),
           format->GetField(JSDateTimeFormat::HOUR12),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("locale"),
           format->GetField(JSDateTimeFormat::LOCALE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("numberingSystem"),
           format->GetField(JSDateTimeFormat::NUMBERING_SYSTEM),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("weekday"),
           format->GetField(JSDateTimeFormat::WEEKDAY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("era"),
           format->GetField(JSDateTimeFormat::ERA),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("year"),
           format->GetField(JSDateTimeFormat::YEAR))
      .def(ctx->Intern("month"),
           format->GetField(JSDateTimeFormat::MONTH),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("day"),
           format->GetField(JSDateTimeFormat::DAY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("hour"),
           format->GetField(JSDateTimeFormat::HOUR),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("minute"),
           format->GetField(JSDateTimeFormat::MINUTE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("second"),
           format->GetField(JSDateTimeFormat::SECOND),
           ATTR::W | ATTR::E | ATTR::C)
      .def(ctx->Intern("timeZoneName"),
           format->GetField(JSDateTimeFormat::TIME_ZONE_NAME),
           ATTR::W | ATTR::E | ATTR::C);
  return obj;
}

JSVal DateTimeFormatSupportedLocalesOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Intl.DateTimeFormat.supportedLocalesOf", args, e);
  int32_t num = 0;
  Context* ctx = args.ctx();
  const icu::Locale* avail = icu::DateFormat::getAvailableLocales(num);
  std::vector<std::string> vec(num);
  for (int32_t i = 0; i < num; ++i) {
    vec[i] = avail[i].getName();
  }
  JSVector* requested =
      i18n::CanonicalizeLocaleList(ctx, args.At(0), IV_LV5_ERROR(e));
  return detail_i18n::SupportedLocales(
      args.ctx(),
      vec.begin(),
      vec.end(),
      requested, args.At(1), e);
}
#endif  // IV_ENABLE_I18N

} } }  // namespace iv::lv5::runtime
