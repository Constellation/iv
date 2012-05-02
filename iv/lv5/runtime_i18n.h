#ifndef IV_LV5_RUNTIME_I18N_H_
#define IV_LV5_RUNTIME_I18N_H_
#ifdef IV_ENABLE_I18N
#include <iv/ignore_unused_variable_warning.h>
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
namespace detail_i18n {

JSLocaleList* CreateLocaleList(Context* ctx,
                               JSVal target, Error* e);

}  // namespace detail_i18n

inline JSLocaleList* detail_i18n::CreateLocaleList(
    Context* ctx, JSVal target, Error* e) {
  std::vector<std::string> list;
  if (target.IsUndefined()) {
    // TODO(Constellation) implement default locale system
    list.push_back("en-US");
  } else {
    JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR_WITH(e, NULL));
    const uint32_t len =
        internal::GetLength(ctx, obj, IV_LV5_ERROR_WITH(e, NULL));
    for (uint32_t k = 0; k < len; ++k) {
      const Symbol name = symbol::MakeSymbolFromIndex(k);
      if (obj->HasProperty(ctx, name)) {
        const JSVal value = obj->Get(ctx, name, IV_LV5_ERROR_WITH(e, NULL));
        if (!(value.IsString() || value.IsObject())) {
          e->Report(Error::Type, "locale should be string or object");
          return NULL;
        }
        JSString* tag = value.ToString(ctx, IV_LV5_ERROR_WITH(e, NULL));
        core::i18n::LanguageTagScanner scanner(tag->begin(), tag->end());
        if (!scanner.IsWellFormed()) {
          e->Report(Error::Range, "locale pattern is not well formed");
          return NULL;
        }
        const std::string canonicalized = scanner.Canonicalize();
        if (std::find(list.begin(), list.end(), canonicalized) == list.end()) {
          list.push_back(canonicalized);
        }
      }
    }
  }

  JSLocaleList* localelist = JSLocaleList::New(ctx);

  uint32_t index = 0;
  for (std::vector<std::string>::const_iterator it = list.begin(),
       last = list.end(); it != last; ++it, ++index) {
    localelist->DefineOwnProperty(
        ctx,
        symbol::MakeSymbolFromIndex(index),
        DataDescriptor(JSString::NewAsciiString(ctx, *it), ATTR::E),
        true, IV_LV5_ERROR_WITH(e, NULL));
  }
  localelist->DefineOwnProperty(
      ctx,
      symbol::length(),
      DataDescriptor(JSVal::UInt32(index), ATTR::NONE),
      true, IV_LV5_ERROR_WITH(e, NULL));
  localelist->MakeInitializedLocaleList();
  return localelist;
}

inline JSVal LocaleListConstructor(const Arguments& args, Error* e) {
  return detail_i18n::CreateLocaleList(args.ctx(), args.At(0), e);
}

class Options {
 public:
  enum Type {
    BOOLEAN,
    STRING,
    NUMBER
  };

  explicit Options(JSObject* options) : options_(options) { }

  JSVal Get(Context* ctx,
            Symbol property, Type type,
            JSArray* values, JSVal fallback, Error* e) {
    JSVal value = options()->Get(ctx, property, IV_LV5_ERROR(e));
    if (!value.IsNullOrUndefined()) {
      switch (type) {
        case BOOLEAN:
          value = JSVal::Bool(value.ToBoolean());
          break;
        case STRING:
          value = value.ToString(ctx, IV_LV5_ERROR(e));
          break;
        case NUMBER:
          value = value.ToNumber(ctx, IV_LV5_ERROR(e));
          break;
      }
      if (values) {
        const JSVal method =
            values->Get(ctx, context::Intern(ctx, "indexOf"), IV_LV5_ERROR(e));
        if (method.IsCallable()) {
          ScopedArguments args(ctx, 1, IV_LV5_ERROR(e));
          args[0] = value;
          const JSVal val =
            method.object()->AsCallable()->Call(&args, values, IV_LV5_ERROR(e));
          if (JSVal::StrictEqual(val, JSVal::Int32(-1))) {
            e->Report(Error::Range, "option out of range");
            return JSEmpty;
          }
        } else {
          e->Report(Error::Range, "option out of range");
          return JSEmpty;
        }
      }
      return value;
    }
    return fallback;
  }

  JSObject* options() { return options_; }

 private:
  JSObject* options_;
};

struct CollatorOption {
  const char* key;
  std::size_t field;
  const char* property;
  Options::Type type;
  std::array<const char*, 4> values;
};

typedef std::array<CollatorOption, 6> CollatorOptionTable;

static const CollatorOptionTable kCollatorOptionTable = { {
  { "kb", JSCollator::BACKWARDS, "backwards",
    Options::BOOLEAN, { { NULL } } },
  { "kc", JSCollator::CASE_LEVEL, "caseLevel",
    Options::BOOLEAN, { { NULL } } },
  { "kn", JSCollator::NUMERIC, "numeric",
    Options::BOOLEAN, { { NULL } } },
  { "kh", JSCollator::HIRAGANA_QUATERNARY, "hiraganaQuaternary",
    Options::BOOLEAN, { { NULL } } },
  { "kk", JSCollator::NORMALIZATION, "normalization",
    Options::BOOLEAN, { { NULL } } },
  { "kf", JSCollator::CASE_FIRST, "caseFirst",
    Options::STRING, { { "upper", "lower", "false", NULL } } },
} };

template<JSCollator::CollatorField TYPE, UColAttribute ATTR>
inline void SetICUCollatorBooleanOption(JSCollator* obj,
                                        icu::Collator* collator, Error* e) {
  UErrorCode status = U_ZERO_ERROR;
  const bool res = obj->GetField(TYPE).ToBoolean();
  collator->setAttribute(ATTR, res ? UCOL_ON : UCOL_OFF, status);
  if (U_FAILURE(status)) {
    e->Report(Error::Type, "icu collator initialization failed");
    return;
  }
}

inline JSVal CollatorConstructor(const Arguments& args, Error* e) {
  const JSVal arg1 = args.At(0);
  const JSVal arg2 = args.At(1);
  Context* ctx = args.ctx();
  JSCollator* obj = JSCollator::New(ctx);
  core::ignore_unused_variable_warning(arg1);

  JSObject* options = NULL;
  if (arg2.IsUndefined()) {
    options = JSObject::New(ctx);
  } else {
    options = arg2.ToObject(ctx, IV_LV5_ERROR(e));
  }

  Options opt(options);

  {
    JSVector* vec = JSVector::New(ctx);
    JSString* sort = JSString::NewAsciiString(ctx, "sort");
    JSString* search = JSString::NewAsciiString(ctx, "search");
    vec->push_back(sort);
    vec->push_back(search);
    const JSVal u = opt.Get(ctx,
                            context::Intern(ctx, "usage"),
                            Options::STRING, vec->ToJSArray(),
                            sort, IV_LV5_ERROR(e));
    assert(u.IsString());
    obj->SetField(JSCollator::USAGE, u);
  }

  {
    JSVector* vec = JSVector::New(ctx);
    JSString* lookup = JSString::NewAsciiString(ctx, "lookup");
    JSString* best_fit = JSString::NewAsciiString(ctx, "best fit");
    vec->push_back(lookup);
    vec->push_back(best_fit);
    const JSVal matcher = opt.Get(ctx,
                                  context::Intern(ctx, "localeMatcher"),
                                  Options::STRING, vec->ToJSArray(),
                                  best_fit, IV_LV5_ERROR(e));
    assert(matcher.IsString());
    obj->SetField(JSCollator::LOCALE_MATCHER, matcher);
  }

  {
    for (CollatorOptionTable::const_iterator it = kCollatorOptionTable.begin(),
         last = kCollatorOptionTable.end(); it != last; ++it) {
      JSArray* ary = NULL;
      if (it->values[0] != NULL) {
        JSVector* vec = JSVector::New(ctx);
        for (const char* const * ptr = it->values.data(); *ptr; ++ptr) {
          vec->push_back(JSString::NewAsciiString(ctx, *ptr));
        }
        ary = vec->ToJSArray();
      }
      JSVal value = opt.Get(ctx,
                            context::Intern(ctx, it->property),
                            it->type, ary,
                            JSUndefined, IV_LV5_ERROR(e));
      if (it->type == Options::BOOLEAN) {
        if (!value.IsUndefined()) {
          if (!value.IsBoolean()) {
            const JSVal s = value.ToString(ctx, IV_LV5_ERROR(e));
            value = JSVal::Bool(
                JSVal::StrictEqual(ctx->global_data()->string_true(), s));
          }
        } else {
          value = JSFalse;
        }
      }
      obj->SetField(it->field, value);
    }
  }

  icu::Locale locale("en_US");  // TODO(Constellation) implement it

  UErrorCode status = U_ZERO_ERROR;
  icu::Collator* collator = icu::Collator::createInstance(locale, status);
  if (U_FAILURE(status)) {
    e->Report(Error::Type, "collator creation failed");
    return JSEmpty;
  }
  obj->set_collator(collator);

  {
    JSVector* vec = JSVector::New(ctx);
    JSString* o_base = JSString::NewAsciiString(ctx, "base");
    JSString* o_accent = JSString::NewAsciiString(ctx, "accent");
    JSString* o_case = JSString::NewAsciiString(ctx, "case");
    JSString* o_variant = JSString::NewAsciiString(ctx, "variant");
    vec->push_back(o_base);
    vec->push_back(o_accent);
    vec->push_back(o_case);
    vec->push_back(o_variant);
    JSVal s = opt.Get(ctx,
                      context::Intern(ctx, "sensitivity"),
                      Options::STRING, vec->ToJSArray(),
                      JSUndefined, IV_LV5_ERROR(e));
    if (s.IsUndefined()) {
      if (JSVal::StrictEqual(obj->GetField(JSCollator::USAGE),
                             JSString::NewAsciiString(ctx, "sort"))) {
        s = o_variant;
      } else {
        // TODO(Constellation) dataLocale
        s = o_base;
      }
    }
    obj->SetField(JSCollator::SENSITIVITY, s);
  }

  {
    const JSVal ip = opt.Get(ctx,
                             context::Intern(ctx, "ignorePunctuation"),
                             Options::BOOLEAN, NULL,
                             JSFalse, IV_LV5_ERROR(e));
    obj->SetField(JSCollator::IGNORE_PUNCTUATION, ip);
  }
  obj->SetField(JSCollator::BOUND_COMPARE, JSUndefined);

  // initialize icu::Collator
  SetICUCollatorBooleanOption<
      JSCollator::BACKWARDS,
      UCOL_FRENCH_COLLATION>(obj, collator, IV_LV5_ERROR(e));
  SetICUCollatorBooleanOption<
      JSCollator::CASE_LEVEL,
      UCOL_CASE_LEVEL>(obj, collator, IV_LV5_ERROR(e));
  SetICUCollatorBooleanOption<
      JSCollator::NUMERIC,
      UCOL_NUMERIC_COLLATION>(obj, collator, IV_LV5_ERROR(e));
  SetICUCollatorBooleanOption<
      JSCollator::HIRAGANA_QUATERNARY,
      UCOL_HIRAGANA_QUATERNARY_MODE>(obj, collator, IV_LV5_ERROR(e));
  SetICUCollatorBooleanOption<
      JSCollator::NORMALIZATION,
      UCOL_NORMALIZATION_MODE>(obj, collator, IV_LV5_ERROR(e));

  {
    const JSVal v = obj->GetField(JSCollator::CASE_FIRST);
    if (v.IsUndefined()) {
      collator->setAttribute(UCOL_CASE_FIRST, UCOL_OFF, status);
    } else {
      assert(v.IsString());
      JSString* s = v.string();
      const std::string str(s->begin(), s->end());
      if (str == "upper") {
        collator->setAttribute(UCOL_CASE_FIRST, UCOL_UPPER_FIRST, status);
      } else if (str == "lower") {
        collator->setAttribute(UCOL_CASE_FIRST, UCOL_LOWER_FIRST, status);
      } else {
        collator->setAttribute(UCOL_CASE_FIRST, UCOL_OFF, status);
      }
    }
    if (U_FAILURE(status)) {
      e->Report(Error::Type, "icu collator initialization failed");
      return JSEmpty;
    }
  }

  {
    const JSVal v = obj->GetField(JSCollator::SENSITIVITY);
    if (v.IsUndefined()) {
      collator->setStrength(icu::Collator::TERTIARY);
    } else {
      assert(v.IsString());
      JSString* s = v.string();
      const std::string str(s->begin(), s->end());
      if (str == "base") {
        collator->setStrength(icu::Collator::PRIMARY);
      } else if (str == "accent") {
        collator->setStrength(icu::Collator::SECONDARY);
      } else if (str == "case") {
        collator->setStrength(icu::Collator::PRIMARY);
        collator->setAttribute(UCOL_CASE_LEVEL, UCOL_ON, status);
      } else {
        collator->setStrength(icu::Collator::TERTIARY);
      }
    }
    if (U_FAILURE(status)) {
      e->Report(Error::Type, "icu collator initialization failed");
      return JSEmpty;
    }
  }

  {
    if (obj->GetField(JSCollator::IGNORE_PUNCTUATION).ToBoolean()) {
      collator->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
      if (U_FAILURE(status)) {
        e->Report(Error::Type, "icu collator initialization failed");
        return JSEmpty;
      }
    }
  }
  return obj;
}

inline JSVal CollatorCompareGetter(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Collator.compare", args, e);
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
  IV_LV5_CONSTRUCTOR_CHECK("Collator.resolvedOptions", args, e);
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
           collator->GetField(JSCollator::IGNORE_PUNCTUATION),
           ATTR::W | ATTR::E | ATTR::C);
  return obj;
}

template<typename AvailIter>
inline JSVal LookupSupportedLocales(
    Context* ctx, AvailIter it, AvailIter last,
    JSLocaleList* list, Error* e) {
  const uint32_t len = internal::GetLength(ctx, list, IV_LV5_ERROR(e));
  std::vector<std::string> subset;
  for (uint32_t k = 0; k < len; ++k) {
    const JSVal res =
        list->Get(ctx, symbol::MakeSymbolFromIndex(k), IV_LV5_ERROR(e));
    // TODO(Constellation) noExtensionsLocale
    JSString* str = res.ToString(ctx, IV_LV5_ERROR(e));
    const std::string locale(str->begin(), str->end());
    const AvailIter t = core::i18n::IndexOfMatch(it, last, locale);
    if (t != last) {
      subset.push_back(locale);
    }
  }

  JSLocaleList* localelist = JSLocaleList::New(ctx);

  uint32_t index = 0;
  for (std::vector<std::string>::const_iterator it = subset.begin(),
       last = subset.end(); it != last; ++it, ++index) {
    localelist->DefineOwnProperty(
        ctx,
        symbol::MakeSymbolFromIndex(index),
        DataDescriptor(JSString::NewAsciiString(ctx, *it), ATTR::E),
        true, IV_LV5_ERROR(e));
  }
  localelist->DefineOwnProperty(
      ctx,
      symbol::length(),
      DataDescriptor(JSVal::UInt32(index), ATTR::NONE),
      true, IV_LV5_ERROR(e));
  localelist->MakeInitializedLocaleList();
  return localelist;
}

template<typename AvailIter>
inline JSVal BestFitSupportedLocales(
    Context* ctx, AvailIter it, AvailIter last,
    JSLocaleList* list, Error* e) {
  return LookupSupportedLocales(ctx, it, last, list, e);
}

template<typename AvailIter>
inline JSVal SupportedLocales(Context* ctx,
                              AvailIter it,
                              AvailIter last,
                              JSVal requested, JSVal options, Error* e) {
  JSLocaleList* list = NULL;
  JSObject* req = requested.ToObject(ctx, IV_LV5_ERROR(e));
  if (!req->IsClass<Class::LocaleList>()) {
    list = static_cast<JSLocaleList*>(requested.object());
  } else {
    list = detail_i18n::CreateLocaleList(ctx, req, IV_LV5_ERROR(e));
  }
  bool best_fit = true;
  if (!options.IsUndefined()) {
    JSObject* opt = options.ToObject(ctx, IV_LV5_ERROR(e));
    const JSVal matcher =
        opt->Get(ctx, context::Intern(ctx, "localeMatcher"), IV_LV5_ERROR(e));
    if (!matcher.IsUndefined()) {
      JSString* str = matcher.ToString(ctx, IV_LV5_ERROR(e));
      if (*str == *JSString::NewAsciiString(ctx, "lookup")) {
        best_fit = false;
      } else if (*str != *JSString::NewAsciiString(ctx, "best fit")) {
        e->Report(Error::Range,
                  "localeMatcher should be 'lookup' or 'best fit'");
        return JSEmpty;
      }
    }
  }
  if (best_fit) {
    return BestFitSupportedLocales(ctx, it, last, list, e);
  } else {
    return LookupSupportedLocales(ctx, it, last, list, e);
  }
}

inline JSVal CollatorSupportedLocalesOf(const Arguments& args, Error* e) {
  return SupportedLocales(
      args.ctx(),
      ICUStringIteration(icu::Collator::getAvailableLocales()),
      ICUStringIteration(),
      args.At(0), args.At(1), e);
}

class NumberOptions : public Options {
 public:
  explicit NumberOptions(JSObject* options) : Options(options) { }

  double GetNumber(Context* ctx,
                   Symbol property,
                   double minimum,
                   double maximum, double fallback, Error* e) {
    const JSVal value =
        options()->Get(ctx, property, IV_LV5_ERROR_WITH(e, 0.0));
    if (!value.IsNullOrUndefined()) {
      const double res = value.ToNumber(ctx, IV_LV5_ERROR_WITH(e, 0.0));
      if (core::math::IsNaN(res) || res < minimum || res > maximum) {
        e->Report(Error::Range, "option out of range");
        return 0.0;
      }
      return std::floor(res);
    }
    return fallback;
  }
};

inline JSVal NumberFormatConstructor(const Arguments& args, Error* e) {
  const JSVal arg1 = args.At(0);
  const JSVal arg2 = args.At(1);
  Context* ctx = args.ctx();
  JSNumberFormat* obj = JSNumberFormat::New(ctx);
  core::ignore_unused_variable_warning(arg1);

  JSObject* options = NULL;
  if (arg2.IsUndefined()) {
    options = JSObject::New(ctx);
  } else {
    options = arg2.ToObject(ctx, IV_LV5_ERROR(e));
  }

  NumberOptions opt(options);

  {
    JSVector* vec = JSVector::New(ctx);
    JSString* lookup = JSString::NewAsciiString(ctx, "lookup");
    JSString* best_fit = JSString::NewAsciiString(ctx, "best fit");
    vec->push_back(lookup);
    vec->push_back(best_fit);
    const JSVal matcher = opt.Get(ctx,
                                  context::Intern(ctx, "localeMatcher"),
                                  Options::STRING, vec->ToJSArray(),
                                  best_fit, IV_LV5_ERROR(e));
    assert(matcher.IsString());
    // TODO(Constellation) fix ResolveLocale
    core::ignore_unused_variable_warning(matcher);
  }

  icu::Locale locale("en_US");  // TODO(Constellation) implement it

  UErrorCode status = U_ZERO_ERROR;
  icu::NumberFormat* format = icu::NumberFormat::createInstance(locale, status);
  if (U_FAILURE(status)) {
    e->Report(Error::Type, "number format creation failed");
    return JSEmpty;
  }
  obj->set_number_format(format);

  enum {
    DECIMAL_STYLE,
    PERCENT_STYLE,
    CURRENCY_STYLE
  } style = DECIMAL_STYLE;
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
                            Options::STRING, vec->ToJSArray(),
                            decimal, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::STYLE, t);
    if (JSVal::StrictEqual(style, currency)) {
      style = CURRENCY_STYLE;
    } else if (JSVal::StrictEqual(style, decimal)) {
      style = DECIMAL_STYLE;
    } else {
      style = PERCENT_STYLE;
    }
  }

  {
    const JSVal currency = opt.Get(ctx,
                                   context::Intern(ctx, "currency"),
                                   Options::STRING, NULL,
                                   JSUndefined, IV_LV5_ERROR(e));
    if (style == CURRENCY_STYLE) {
      if (currency.IsUndefined()) {
        e->Report(Error::Type, "currency is not specified");
        return JSEmpty;
      }
      // TODO(Constellation) once check uppercase and if found transform it
      JSString* tmp = currency.string();
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
    }
  }

  {
    JSVector* vec = JSVector::New(ctx);
    JSString* code = JSString::NewAsciiString(ctx, "code");
    JSString* symbol = JSString::NewAsciiString(ctx, "symbol");
    JSString* name = JSString::NewAsciiString(ctx, "name");
    vec->push_back(code);
    vec->push_back(symbol);
    vec->push_back(name);
    const JSVal cd = opt.Get(ctx,
                             context::Intern(ctx, "currencyDisplay"),
                             Options::STRING, vec->ToJSArray(),
                             symbol, IV_LV5_ERROR(e));
    if (style == CURRENCY_STYLE) {
      obj->SetField(JSNumberFormat::CURRENCY_DISPLAY, cd);
    }
  }

  const double minimum_integer_digits =
      opt.GetNumber(ctx,
                    context::Intern(ctx, "minimumIntegerDigits"),
                    1,
                    21,
                    1, IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::MINIMUM_INTEGER_DIGITS,
                minimum_integer_digits);

  // TODO(Constellation) use CurrencyDigits(c)
  const double minimum_fraction_digits_default = 0;

  const double minimum_fraction_digits =
      opt.GetNumber(ctx,
                    context::Intern(ctx, "minimumFractionDigits"),
                    0,
                    20,
                    minimum_fraction_digits_default, IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::MINIMUM_FRACTION_DIGITS,
                minimum_fraction_digits);

  // TODO(Constellation) use CurrencyDigits(c)
  const double maximum_fraction_digits_default =
      (style == CURRENCY_STYLE) ?
        (std::max)(minimum_fraction_digits_default, 3.0) :
      (style == PERCENT_STYLE) ?
        (std::max)(minimum_fraction_digits_default, 0.0) :
        (std::max)(minimum_fraction_digits_default, 3.0);

  const double maximum_fraction_digits =
      opt.GetNumber(ctx,
                    context::Intern(ctx, "maximumFractionDigits"),
                    minimum_fraction_digits,
                    20,
                    maximum_fraction_digits_default, IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::MAXIMUM_FRACTION_DIGITS,
                maximum_fraction_digits);

  if (options->HasProperty(ctx,
                           context::Intern(ctx, "minimumSignificantDigits")) ||
      options->HasProperty(ctx,
                           context::Intern(ctx, "maximumSignificantDigits"))) {
    const double minimum_significant_digits =
        opt.GetNumber(ctx,
                      context::Intern(ctx, "minimumSignificantDigits"),
                      1,
                      21,
                      1, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::MINIMUM_SIGNIFICANT_DIGITS,
                  minimum_significant_digits);

    const double maximum_significant_digits =
        opt.GetNumber(ctx,
                      context::Intern(ctx, "maximumSignificantDigits"),
                      minimum_significant_digits,
                      21,
                      21, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::MAXIMUM_SIGNIFICANT_DIGITS,
                  maximum_significant_digits);
  }

  {
    const JSVal ug = opt.Get(ctx,
                             context::Intern(ctx, "useGrouping"),
                             Options::BOOLEAN, NULL,
                             JSTrue, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::USE_GROUPING, ug);
  }
  return obj;
}

inline JSVal NumberFormatSupportedLocalesOf(const Arguments& args, Error* e) {
  return SupportedLocales(
      args.ctx(),
      ICUStringIteration(icu::NumberFormat::getAvailableLocales()),
      ICUStringIteration(),
      args.At(0), args.At(1), e);
}

#undef IV_LV5_I18N_INTL_CHECK
} } }  // namespace iv::lv5::runtime
#endif  // IV_ENABLE_I18N_H_
#endif  // IV_LV5_RUNTIME_I18N_H_
