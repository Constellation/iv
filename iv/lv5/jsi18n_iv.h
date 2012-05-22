#ifndef IV_LV5_JSI18N_IV_H_
#define IV_LV5_JSI18N_IV_H_
namespace iv {
namespace lv5 {

class JSNumberFormat : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(NumberFormat)

  enum NumberFormatField {
    STYLE = 0,
    CURRENCY,
    CURRENCY_DISPLAY,
    MINIMUM_INTEGER_DIGITS,
    MINIMUM_FRACTION_DIGITS,
    MAXIMUM_FRACTION_DIGITS,
    MINIMUM_SIGNIFICANT_DIGITS,
    MAXIMUM_SIGNIFICANT_DIGITS,
    USE_GROUPING,
    POSITIVE_PATTERN,
    NEGATIVE_PATTERN,
    LOCALE,
    NUMBERING_SYSTEM,
    NUM_OF_FIELDS
  };

  typedef JSIntl::GCHandle<core::i18n::NumberFormat> FormatHandle;

  explicit JSNumberFormat(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      format_(new FormatHandle()),
      fields_() {
  }

  JSNumberFormat(Context* ctx, Map* map)
    : JSObject(map),
      format_(new FormatHandle()),
      fields_() {
  }

  JSVal GetField(std::size_t n) {
    assert(n < fields_.size());
    return fields_[n];
  }

  void SetField(std::size_t n, JSVal v) {
    assert(n < fields_.size());
    fields_[n] = v;
  }

  void set_format(core::i18n::NumberFormat* number_format) {
    format_->handle.reset(number_format);
  }

  core::i18n::NumberFormat* format() const {
    return format_->handle.get();
  }

  JSVal Format(Context* ctx, double value, Error* e) {
    if (value == 0) {
      // if value is -0, we overwrite it as +0
      value = 0;
    }
    return JSString::New(ctx, format()->Format(value));
  }

  static JSNumberFormat* New(Context* ctx) {
    JSNumberFormat* const collator = new JSNumberFormat(ctx);
    collator->set_cls(JSNumberFormat::GetClass());
    collator->set_prototype(
        context::GetClassSlot(ctx, Class::NumberFormat).prototype);
    return collator;
  }

  static JSNumberFormat* NewPlain(Context* ctx, Map* map) {
    JSNumberFormat* obj = new JSNumberFormat(ctx, map);
    Error e;
    JSNumberFormat::Initialize(ctx, obj, JSUndefined, JSUndefined, &e);
    return obj;
  }

  static JSVal Initialize(Context* ctx,
                          JSNumberFormat* obj,
                          JSVal requested_locales,
                          JSVal op,
                          Error* e);

  static JSVal SupportedLocalesOf(Context* ctx,
                                  JSVal requested, JSVal options, Error* e);

 private:
  FormatHandle* format_;
  std::array<JSVal, NUM_OF_FIELDS> fields_;
};

inline JSVal JSNumberFormat::Initialize(Context* ctx,
                                        JSNumberFormat* obj,
                                        JSVal requested_locales,
                                        JSVal op,
                                        Error* e) {
  JSObject* options = NULL;
  if (op.IsUndefined()) {
    options = JSObject::New(ctx);
  } else {
    options = op.ToObject(ctx, IV_LV5_ERROR(e));
  }

  detail_i18n::NumberOptions opt(options);

  const core::i18n::LookupResult result =
      detail_i18n::ResolveLocale(
          ctx,
          core::i18n::NumberFormat::AvailableLocales().begin(),
          core::i18n::NumberFormat::AvailableLocales().end(),
          requested_locales,
          options,
          IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::LOCALE,
                JSString::NewAsciiString(ctx, result.locale()));
  const core::i18n::NumberFormat::Data* locale =
      core::i18n::NumberFormat::Lookup(result.locale());

  if (!locale) {
    e->Report(Error::Range, "locale not supported");
    return JSEmpty;
  }

  core::i18n::NumberFormat::Style style = core::i18n::NumberFormat::DECIMAL;
  core::i18n::Currency::Display display = core::i18n::Currency::SYMBOL;
  const core::i18n::Currency::Data* currency_data = NULL;
  {
    JSString* decimal = JSString::NewAsciiString(ctx, "decimal");
    JSString* percent = JSString::NewAsciiString(ctx, "percent");
    JSString* currency = JSString::NewAsciiString(ctx, "currency");
    JSVector* vec = JSVector::New(ctx);
    vec->push_back(decimal);
    vec->push_back(percent);
    vec->push_back(currency);
    const JSVal t = opt.Get(ctx,
                            context::Intern(ctx, "style"),
                            detail_i18n::Options::STRING, vec,
                            decimal, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::STYLE, t);

    if (JSVal::StrictEqual(t, currency)) {
      style = core::i18n::NumberFormat::CURRENCY;
      {
        // currency option
        const JSVal currency = opt.Get(ctx,
                                       context::Intern(ctx, "currency"),
                                       detail_i18n::Options::STRING, NULL,
                                       JSUndefined, IV_LV5_ERROR(e));
        if (currency.IsUndefined()) {
          e->Report(Error::Type, "currency is not specified");
          return JSEmpty;
        }
        JSString* tmp = currency.string();
        if (!core::i18n::IsWellFormedCurrencyCode(tmp->begin(), tmp->end())) {
          e->Report(Error::Range, "invalid currency code");
          return JSEmpty;
        }
        JSStringBuilder builder;
        for (JSString::const_iterator it = tmp->begin(), last = tmp->end();
             it != last; ++it) {
          if (core::character::IsASCII(*it)) {
            builder.Append(core::character::ToUpperCase(*it));
          } else {
            builder.Append(*it);
          }
        }
        JSString* c = builder.Build(ctx);
        obj->SetField(JSNumberFormat::CURRENCY, c);

        // check target currency exists
        {
          const std::string currency(c->begin(), c->end());
          currency_data = core::i18n::Currency::Lookup(currency);
          if (!currency_data) {
            e->Report(Error::Range, "target currency code does not exists");
            return JSEmpty;
          }
        }
      }
      {
        // currencyDisplay option
        JSVector* vec = JSVector::New(ctx);
        JSString* code = JSString::NewAsciiString(ctx, "code");
        JSString* symbol = JSString::NewAsciiString(ctx, "symbol");
        JSString* name = JSString::NewAsciiString(ctx, "name");
        vec->push_back(code);
        vec->push_back(symbol);
        vec->push_back(name);
        const JSVal cd = opt.Get(ctx,
                                 context::Intern(ctx, "currencyDisplay"),
                                 detail_i18n::Options::STRING, vec,
                                 symbol, IV_LV5_ERROR(e));
        obj->SetField(JSNumberFormat::CURRENCY_DISPLAY, cd);
        display =
            (JSVal::StrictEqual(cd, code)) ?
              core::i18n::Currency::CODE :
            (JSVal::StrictEqual(cd, name)) ?
              core::i18n::Currency::NAME : core::i18n::Currency::SYMBOL;
      }
    } else if (JSVal::StrictEqual(t, decimal)) {
      style = core::i18n::NumberFormat::DECIMAL;
    } else {
      style = core::i18n::NumberFormat::PERCENT;
    }
  }

  const int32_t minimum_integer_digits =
      opt.GetNumber(ctx,
                    context::Intern(ctx, "minimumIntegerDigits"),
                    1,
                    21,
                    1, IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::MINIMUM_INTEGER_DIGITS,
                JSVal::Int32(minimum_integer_digits));

  const int32_t minimum_fraction_digits_default =
      (style == core::i18n::NumberFormat::CURRENCY) ?
      currency_data->CurrencyDigits(): 0;

  const int32_t minimum_fraction_digits =
      opt.GetNumber(ctx,
                    context::Intern(ctx, "minimumFractionDigits"),
                    0,
                    20,
                    minimum_fraction_digits_default, IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::MINIMUM_FRACTION_DIGITS,
                JSVal::Int32(minimum_fraction_digits));

  const int32_t maximum_fraction_digits_default =
      (style == core::i18n::NumberFormat::CURRENCY) ?
        (std::max)(minimum_fraction_digits_default, currency_data->CurrencyDigits()) :
      (style == core::i18n::NumberFormat::PERCENT) ?
        (std::max)(minimum_fraction_digits_default, 0) :
        (std::max)(minimum_fraction_digits_default, 3);
  const int32_t maximum_fraction_digits =
      opt.GetNumber(ctx,
                    context::Intern(ctx, "maximumFractionDigits"),
                    minimum_fraction_digits,
                    20,
                    maximum_fraction_digits_default, IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::MAXIMUM_FRACTION_DIGITS,
                JSVal::Int32(maximum_fraction_digits));

  int32_t minimum_significant_digits = core::i18n::NumberFormat::kUnspecified;
  int32_t maximum_significant_digits = core::i18n::NumberFormat::kUnspecified;
  if (options->HasProperty(ctx,
                           context::Intern(ctx, "minimumSignificantDigits")) ||
      options->HasProperty(ctx,
                           context::Intern(ctx, "maximumSignificantDigits"))) {
    minimum_significant_digits =
        opt.GetNumber(ctx,
                      context::Intern(ctx, "minimumSignificantDigits"),
                      1,
                      21,
                      1, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::MINIMUM_SIGNIFICANT_DIGITS,
                  JSVal::Int32(minimum_significant_digits));
    maximum_significant_digits =
        opt.GetNumber(ctx,
                      context::Intern(ctx, "maximumSignificantDigits"),
                      minimum_significant_digits,
                      21,
                      21, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::MAXIMUM_SIGNIFICANT_DIGITS,
                  JSVal::Int32(maximum_significant_digits));
  }

  {
    const JSVal ug = opt.Get(ctx,
                             context::Intern(ctx, "useGrouping"),
                             detail_i18n::Options::BOOLEAN, NULL,
                             JSTrue, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::USE_GROUPING, ug);
  }

  obj->set_format(
      new core::i18n::NumberFormat(
          locale,
          style,
          minimum_significant_digits,
          maximum_significant_digits,
          minimum_integer_digits,
          minimum_fraction_digits,
          maximum_fraction_digits,
          NULL,
          currency_data,
          display
  ));
  return obj;
}

inline JSVal JSNumberFormat::SupportedLocalesOf(Context* ctx, JSVal requested, JSVal options, Error* e) {
  return detail_i18n::SupportedLocales(
      ctx,
      core::i18n::NumberFormat::AvailableLocales().begin(),
      core::i18n::NumberFormat::AvailableLocales().end(),
      requested, options, e);
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSI18N_IV_H_
