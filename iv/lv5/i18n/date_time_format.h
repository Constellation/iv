#ifndef IV_LV5_I18N_DATE_TIME_FORMAT_H_
#define IV_LV5_I18N_DATE_TIME_FORMAT_H_
#include <iv/i18n.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/bind.h>
#include <iv/lv5/context_fwd.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/i18n/utility.h>
namespace iv {
namespace lv5 {
namespace i18n {

class JSDateTimeFormatHolder : public JSObject {
 public:
  typedef GCHandle<core::i18n::DateTimeFormat> FormatHandle;

  explicit JSDateTimeFormatHolder(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      format_(new FormatHandle()),
      bound_(NULL) { }

  static JSDateTimeFormatHolder* New(Context* ctx,
                                     core::i18n::DateTimeFormat* number_format) {
    JSDateTimeFormatHolder* const format = new JSDateTimeFormatHolder(ctx);
    format->set_cls(JSObject::GetClass());
    format->set_prototype(ctx->global_data()->object_prototype());
    format->set_format(number_format);
    return format;
  }

  static JSDateTimeFormatHolder* Extract(Context* ctx, JSObject* obj) {
    Slot slot;
    const bool res = obj->GetOwnPropertySlot(
        ctx, ctx->i18n()->symbols().initializedDateTimeFormat(), &slot);
    core::ignore_unused_variable_warning(res);
    assert(res);
    return static_cast<JSDateTimeFormatHolder*>(slot.value().object());
  }

  core::i18n::DateTimeFormat* format() const {
    return format_->handle.get();
  }

  JSVal Format(Context* ctx, double value, Error* e) {
    if (!core::math::IsFinite(value)) {
      e->Report(Error::Range, "target date value is not finite number");
    }
    return JSString::New(ctx, format()->Format(value), e);
  }

  JSFunction* Bound(Context* ctx, Error* e);

 private:
  void set_format(core::i18n::DateTimeFormat* number_format) {
    format_->handle.reset(number_format);
  }

  FormatHandle* format_;
  JSFunction* bound_;
};

class JSDateTimeFormatBoundFunction : public JSFunction {
 public:
  virtual JSVal Call(Arguments* args, JSVal this_binding, Error* e) {
    const double value = args->At(0).ToNumber(args->ctx(), IV_LV5_ERROR(e));
    return format_->Format(args->ctx(), value, e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    e->Report(Error::Type, "Intl.NumberFormat bound function does not have [[Construct]]");
    return JSEmpty;
  }

  virtual bool IsNativeFunction() const { return true; }

  virtual bool IsBoundFunction() const { return false; }

  virtual bool IsStrict() const { return false; }

  virtual JSAPI NativeFunction() const { return NULL; }

  static JSDateTimeFormatBoundFunction* New(Context* ctx, JSDateTimeFormatHolder* format) {
    JSDateTimeFormatBoundFunction* const obj =
        new JSDateTimeFormatBoundFunction(ctx, format);
    obj->Initialize(ctx);
    return obj;
  }

 private:
  explicit JSDateTimeFormatBoundFunction(Context* ctx, JSDateTimeFormatHolder* format)
    : JSFunction(ctx),
      format_(format) {
    Error::Dummy dummy;
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(JSVal::UInt32(1u), ATTR::NONE), false, NULL);
    DefineOwnProperty(
        ctx, symbol::name(),
        DataDescriptor(
            JSString::NewAsciiString(ctx, "format", &dummy),
            ATTR::NONE), false, NULL);
  }

  JSDateTimeFormatHolder* format_;
};

// 12.1.1.1 InitializeDateTimeFormat(numberFormat, locales, options)
inline JSObject* InitializeDateTimeFormat(Context* ctx,
                                          JSObject* format,
                                          JSVal locales, JSVal op, Error* e) {
  if (format->HasOwnProperty(ctx, ctx->i18n()->symbols().initializedIntlObject())) {
    e->Report(Error::Type, "object has been already initialized as Intl group object");
    return NULL;
  }

  format->DefineOwnProperty(
      ctx,
      ctx->i18n()->symbols().initializedIntlObject(),
      DataDescriptor(JSTrue, ATTR::N),
      false, IV_LV5_ERROR_WITH(e, NULL));

  JSVector* requested_locales = CanonicalizeLocaleList(ctx, locales, IV_LV5_ERROR_WITH(e, NULL));

  JSObject* o = ToDateTimeOptions(ctx, op, DATE_ANY, DATE_DATE, IV_LV5_ERROR_WITH(e, NULL));

  JSObject* opt = JSObject::New(ctx);

  Options options(o);

  // TODO(Constellation)
  // clean up this duplicated code
  static const std::array<core::StringPiece, 2> k6 = { {
    "lookup",
    "best fit"
  } };
  JSString* matcher = options.GetString(ctx, symbol::localeMatcher(), k6.begin(), k6.end(), "best fit", IV_LV5_ERROR_WITH(e, NULL));

  opt->DefineOwnProperty(
      ctx,
      symbol::localeMatcher(),
      DataDescriptor(matcher, ATTR::N),
      false, IV_LV5_ERROR_WITH(e, NULL));

  const core::i18n::LookupResult result =
      detail_i18n::ResolveLocale(
          ctx,
          core::i18n::DateTimeFormat::AvailableLocales().begin(),
          core::i18n::DateTimeFormat::AvailableLocales().end(),
          requested_locales,
          opt,
          IV_LV5_ERROR_WITH(e, NULL));

  // get [[localeData]]
  const core::i18n::DateTimeFormat::Data* locale =
      core::i18n::DateTimeFormat::Lookup(result.locale());

  // TODO(Constellation)
  // nu and calendar

  core::i18n::TimeZone::Type tz = core::i18n::TimeZone::UNSPECIFIED;
  {
    const JSVal time_zone =
        options.options()->Get(ctx, symbol::timeZone(), IV_LV5_ERROR_WITH(e, NULL));
    if (!time_zone.IsUndefined()) {
      JSString* str = time_zone.ToString(ctx, IV_LV5_ERROR_WITH(e, NULL));
      std::vector<uint16_t> vec;
      for (JSString::const_iterator it = str->begin(),
           last = str->end(); it != last; ++it) {
        vec.push_back(core::i18n::ToLocaleIdentifierUpperCase(*it));
      }
      if (vec.size() != 3 || vec[0] != 'U' || vec[1] != 'T' || vec[2] != 'C') {
        e->Report(Error::Range, "timeZone is not UTC");
        return NULL;
      }
      tz = core::i18n::TimeZone::UTC;
    }
  }

  const bool hour12 = options.GetBoolean(ctx, symbol::hour12(), locale->hour12, IV_LV5_ERROR_WITH(e, NULL));

  core::i18n::DateTimeFormat::FormatOptions set = { {  } };
  {
    std::size_t i = 0;
    for (core::i18n::DateTimeFormat::DateTimeOptions::const_iterator
         it = core::i18n::kDateTimeOptions.begin(),
         last = core::i18n::kDateTimeOptions.end();
         it != last; ++it, ++i) {
      const Symbol prop = ctx->Intern(it->key);
      JSString* str =
          options.GetString(
              ctx, prop,
              it->values.begin(), it->values.begin() + it->size,
              NULL, IV_LV5_ERROR_WITH(e, NULL));
      if (str) {
        set[i] =
            core::i18n::DateTimeFormat::ToFormatValue(str->begin(), str->end());
      }
    }
  }

  // clean up this duplicated code
  static const std::array<core::StringPiece, 2> k24 = { {
    "basic",
    "best fit"
  } };
  JSString* format_matcher = options.GetString(
      ctx, symbol::formatMatcher(), k24.begin(), k24.end(), "best fit", IV_LV5_ERROR_WITH(e, NULL));

  core::i18n::DateTimeFormat::FormatOptions best_format = { { } };
  if (format_matcher->compare("basic") == 0) {
    BasicFormatMatcher(ctx, set, *locale, &best_format, IV_LV5_ERROR_WITH(e, NULL));
  } else {
    BestFitFormatMatcher(ctx, set, *locale, &best_format, IV_LV5_ERROR_WITH(e, NULL));
  }

  // step 28 is done in best_format

  JSObject* f =
      JSDateTimeFormatHolder::New(
          ctx,
          new core::i18n::DateTimeFormat(
              ctx->i18n(),
              locale,
              hour12,
              tz,
              core::i18n::Calendar::GREGORY,  // TODO(Constellation) specify calendar
              best_format));

  format->DefineOwnProperty(
      ctx,
      ctx->i18n()->symbols().initializedDateTimeFormat(),
      DataDescriptor(f, ATTR::N),
      false, IV_LV5_ERROR_WITH(e, NULL));

  return format;
}

} } }  // namespace iv::lv5::i18n
#endif  // IV_LV5_I18N_DATE_TIME_FORMAT_H_
