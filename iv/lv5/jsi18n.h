// Currently, we implement them by ICU.
// So this is enabled when IV_ENABLE_I18N flag is enabled and ICU is provided.
#ifndef IV_LV5_JSI18N_H_
#define IV_LV5_JSI18N_H_
#ifdef IV_ENABLE_I18N
#include <iterator>
#include <string>
#include <unicode/coll.h>
#include <unicode/decimfmt.h>
#include <unicode/datefmt.h>
#include <unicode/numsys.h>
#include <unicode/dtptngen.h>
#include <unicode/smpdtfmt.h>
#include <iv/detail/unique_ptr.h>
#include <iv/detail/unordered_map.h>
#include <iv/i18n.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
#include <iv/lv5/internal.h>
#include <iv/detail/unique_ptr.h>

#define IV_ICU_VERSION (U_ICU_VERSION_MAJOR_NUM * 10000 + U_ICU_VERSION_MINOR_NUM * 100 + U_ICU_VERSION_PATCHLEVEL_NUM)  // NOLINT
namespace iv {
namespace lv5 {

class ICUStringIteration
  : public std::iterator<std::forward_iterator_tag, const char*> {
 public:
  typedef ICUStringIteration this_type;

  explicit ICUStringIteration(icu::StringEnumeration* enumeration)
    : enumeration_(enumeration) {
    Next();
  }

  ICUStringIteration()
    : enumeration_(NULL),
      current_(NULL) {
  }

  ICUStringIteration(const ICUStringIteration& rhs)
    : enumeration_((rhs.enumeration_) ? rhs.enumeration_->clone() : NULL),
      current_(rhs.current_) {
  }

  this_type& operator++() {
    Next();
    return *this;
  }

  this_type operator++(int) {  // NOLINT
    const this_type temp(*this);
    Next();
    return temp;
  }

  const char* operator*() const {
    return current_;
  }

  this_type& operator=(const this_type& rhs) {
    this_type tmp(rhs);
    tmp.swap(*this);
    return *this;
  }

  bool operator==(const this_type& rhs) const {
    if (enumeration_) {
      if (!rhs.enumeration_) {
        return false;
      }
      return *enumeration_ == *rhs.enumeration_;
    } else {
      return enumeration_ == rhs.enumeration_;
    }
  }

  bool operator!=(const this_type& rhs) const {
      return !(operator==(rhs));
  }

  friend void swap(this_type& lhs, this_type& rhs) {
    using std::swap;
    swap(lhs.enumeration_, rhs.enumeration_);
    swap(lhs.current_, rhs.current_);
  }

  void swap(this_type& rhs) {
    using std::swap;
    swap(*this, rhs);
  }

 private:
  void Next() {
    if (enumeration_) {
      UErrorCode status = U_ZERO_ERROR;
      current_ = enumeration_->next(NULL, status);
      if (U_FAILURE(status)) {
        current_ = NULL;
      }
      if (!current_) {
        enumeration_.reset();
      }
    }
  }

  core::unique_ptr<icu::StringEnumeration> enumeration_;
  const char* current_;
};

class JSIntl : public JSObject {
 public:
  template<typename T>
  class GCHandle : public gc_cleanup {
   public:
    core::unique_ptr<T> handle;
  };

  IV_LV5_DEFINE_JSCLASS(Intl)
};

class JSLocaleList : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(LocaleList)

  explicit JSLocaleList(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)) {
  }

  JSLocaleList(Context* ctx, Map* map)
    : JSObject(map) {
  }

  static JSLocaleList* New(Context* ctx) {
    JSLocaleList* const localelist = new JSLocaleList(ctx);
    localelist->set_cls(JSLocaleList::GetClass());
    localelist->set_prototype(
        context::GetClassSlot(ctx, Class::LocaleList).prototype);
    return localelist;
  }
};

class JSCollator : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(Collator)

  enum CollatorField {
    USAGE = 0,
    NUMERIC,
    NORMALIZATION,
    CASE_FIRST,
    SENSITIVITY,
    IGNORE_PUNCTUATION,
    BOUND_COMPARE,
    LOCALE,
    COLLATION,
    NUM_OF_FIELDS
  };

  explicit JSCollator(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      collator_fields_(),
      collator_(new JSIntl::GCHandle<icu::Collator>()) {
  }

  JSCollator(Context* ctx, Map* map)
    : JSObject(map),
      collator_fields_(),
      collator_(new JSIntl::GCHandle<icu::Collator>()) {
  }

  static JSCollator* New(Context* ctx) {
    JSCollator* const collator = new JSCollator(ctx);
    collator->set_cls(JSCollator::GetClass());
    collator->set_prototype(
        context::GetClassSlot(ctx, Class::Collator).prototype);
    return collator;
  }

  static JSCollator* NewPlain(Context* ctx, Map* map) {
    JSCollator* obj = new JSCollator(ctx, map);
    Error e;
    JSCollator::Initialize(ctx, obj, JSUndefined, JSUndefined, &e);
    return obj;
  }

  JSVal GetField(std::size_t n) {
    assert(n < collator_fields_.size());
    return collator_fields_[n];
  }

  void SetField(std::size_t n, JSVal v) {
    assert(n < collator_fields_.size());
    collator_fields_[n] = v;
  }

  void set_collator(icu::Collator* collator) {
    collator_->handle.reset(collator);
  }

  icu::Collator* collator() const { return collator_->handle.get(); }

  JSVal Compare(Context* ctx, JSVal left, JSVal right, Error* e) {
    const core::UString lhs = left.ToUString(ctx, IV_LV5_ERROR(e));
    const core::UString rhs = right.ToUString(ctx, IV_LV5_ERROR(e));
    const UChar* lstr = reinterpret_cast<const UChar*>(lhs.data());
    const UChar* rstr = reinterpret_cast<const UChar*>(rhs.data());
    UErrorCode status = U_ZERO_ERROR;
    UCollationResult result =
        collator()->compare(lstr, lhs.size(), rstr, rhs.size(), status);
    if (U_FAILURE(status)) {
      e->Report(Error::Type, "collator compare failed");
      return JSEmpty;
    }
    return JSVal::Int32(static_cast<int32_t>(result));
  }

  static JSVal Initialize(Context* ctx,
                          JSCollator* obj,
                          JSVal requested_locales,
                          JSVal op,
                          Error* e);

 private:
  std::array<JSVal, NUM_OF_FIELDS> collator_fields_;
  JSIntl::GCHandle<icu::Collator>* collator_;
};

class JSCollatorBoundFunction : public JSFunction {
 public:
  virtual JSVal Call(Arguments* args, const JSVal& this_binding, Error* e) {
    return collator_->Compare(args->ctx(), args->At(0), args->At(1), e);
  }

  virtual JSVal Construct(Arguments* args, Error* e) {
    e->Report(Error::Type,
              "collator bound function does not have [[Construct]]");
    return JSEmpty;
  }

  virtual bool IsNativeFunction() const { return true; }

  virtual bool IsBoundFunction() const { return false; }

  virtual bool IsStrict() const { return false; }

  virtual JSAPI NativeFunction() const { return NULL; }

  static JSCollatorBoundFunction* New(Context* ctx, JSCollator* collator) {
    JSCollatorBoundFunction* const obj =
        new JSCollatorBoundFunction(ctx, collator);
    obj->Initialize(ctx);
    return obj;
  }

 private:
  explicit JSCollatorBoundFunction(Context* ctx, JSCollator* collator)
    : JSFunction(ctx),
      collator_(collator) {
    DefineOwnProperty(
        ctx, symbol::length(),
        DataDescriptor(JSVal::UInt32(2u), ATTR::NONE), false, NULL);
    DefineOwnProperty(
        ctx, symbol::name(),
        DataDescriptor(
            JSString::NewAsciiString(ctx, "compare"),
            ATTR::NONE), false, NULL);
  }

  JSCollator* collator_;
};

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

  explicit JSNumberFormat(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      number_format_(new JSIntl::GCHandle<icu::DecimalFormat>()),
      fields_() {
  }

  JSNumberFormat(Context* ctx, Map* map)
    : JSObject(map),
      number_format_(new JSIntl::GCHandle<icu::DecimalFormat>()),
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

  void set_number_format(icu::DecimalFormat* number_format) {
    number_format_->handle.reset(number_format);
  }

  icu::DecimalFormat* number_format() const {
    return number_format_->handle.get();
  }

  JSVal Format(Context* ctx, double value, Error* e) {
    UErrorCode err = U_ZERO_ERROR;
    icu::UnicodeString result;
    if (value == 0) {
      // if value is -0, we overwrite it as +0
      value = 0;
    }
    number_format()->format(value, result);
    if (U_FAILURE(err)) {
      e->Report(Error::Type, "Intl.NumberFormat parse failed");
      return JSEmpty;
    }
    return JSString::New(
        ctx, result.getBuffer(), result.getBuffer() + result.length());
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

 private:
  JSIntl::GCHandle<icu::DecimalFormat>* number_format_;
  std::array<JSVal, NUM_OF_FIELDS> fields_;
};

class JSDateTimeFormat : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(DateTimeFormat)

  enum DateTimeFormatField {
    CALENDAR = 0,
    TIME_ZONE,
    HOUR12,
    PATTERN,
    LOCALE,
    NUMBERING_SYSTEM,
    WEEKDAY,
    ERA,
    YEAR,
    MONTH,
    DAY,
    HOUR,
    MINUTE,
    SECOND,
    TIME_ZONE_NAME,
    FORMAT_MATCH,
    NUM_OF_FIELDS
  };

  explicit JSDateTimeFormat(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      date_time_format_(new JSIntl::GCHandle<icu::DateFormat>()),
      fields_() {
  }

  JSDateTimeFormat(Context* ctx, Map* map)
    : JSObject(map),
      date_time_format_(new JSIntl::GCHandle<icu::DateFormat>()),
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

  void set_date_time_format(icu::DateFormat* format) {
    date_time_format_->handle.reset(format);
  }

  icu::DateFormat* date_time_format() const {
    return date_time_format_->handle.get();
  }

  JSVal Format(Context* ctx, double value, Error* e) {
    UErrorCode err = U_ZERO_ERROR;
    icu::UnicodeString result;
    date_time_format()->format(value, result);
    if (U_FAILURE(err)) {
      e->Report(Error::Type, "Intl.DateTimeFormat parse failed");
      return JSEmpty;
    }
    return JSString::New(
        ctx, result.getBuffer(), result.getBuffer() + result.length());
  }

  static JSDateTimeFormat* New(Context* ctx) {
    JSDateTimeFormat* const collator = new JSDateTimeFormat(ctx);
    collator->set_cls(JSDateTimeFormat::GetClass());
    collator->set_prototype(
        context::GetClassSlot(ctx, Class::DateTimeFormat).prototype);
    return collator;
  }

  static JSDateTimeFormat* NewPlain(Context* ctx, Map* map) {
    JSDateTimeFormat* obj = new JSDateTimeFormat(ctx, map);
    Error e;
    JSDateTimeFormat::Initialize(ctx, obj, JSUndefined, JSUndefined, &e);
    return obj;
  }

  static JSVal Initialize(Context* ctx,
                          JSDateTimeFormat* obj,
                          JSVal requested_locales,
                          JSVal op,
                          Error* e);


 private:
  JSIntl::GCHandle<icu::DateFormat>* date_time_format_;
  std::array<JSVal, NUM_OF_FIELDS> fields_;
};

namespace detail_i18n {

inline JSLocaleList* CreateLocaleList(
    Context* ctx, JSVal target, Error* e) {
  std::vector<std::string> list;
  if (target.IsUndefined()) {
    // const Locale& locale = icu::Locale::getDefault();
    // list.push_back(locale.getName());

    // default locale
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
        if (!scanner.IsStructurallyValid()) {
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
  return localelist;
}

template<typename AvailIter>
inline JSVal LookupSupportedLocales(
    Context* ctx, AvailIter it, AvailIter last,
    JSLocaleList* list, Error* e) {
  std::vector<std::string> subset;
  {
    const uint32_t len = internal::GetLength(ctx, list, IV_LV5_ERROR(e));
    for (uint32_t k = 0; k < len; ++k) {
      const JSVal res =
          list->Get(ctx, symbol::MakeSymbolFromIndex(k), IV_LV5_ERROR(e));
      JSString* str = res.ToString(ctx, IV_LV5_ERROR(e));
      const std::string locale(
          core::i18n::LanguageTagScanner::RemoveExtension(str->begin(),
                                                          str->end()));
      const AvailIter t = core::i18n::IndexOfMatch(it, last, locale);
      if (t != last) {
        subset.push_back(locale);
      }
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

typedef std::unordered_map<std::string, std::string> ExtensionMap;

inline ExtensionMap CreateExtensionMap(
    const core::i18n::Locale::Map& extensions) {
  ExtensionMap map;
  core::i18n::Locale::Map::const_iterator it = extensions.find('u');
  if (it == extensions.end()) {
    return map;
  }
  std::string key;
  for (core::i18n::Locale::Map::const_iterator last = extensions.end();
       it != last && it->first == 'u'; ++it) {
    if (it->second.size() == 3) {
      key = it->second;
      map[key] = "";
    } else if (!key.empty()) {
      map[key] = it->second;
      key.clear();
    }
  }
  return map;
}

template<typename AvailIter>
inline core::i18n::LookupResult ResolveLocale(Context* ctx,
                                              AvailIter it,
                                              AvailIter last,
                                              JSVal requested,
                                              JSVal options, Error* e) {
  JSLocaleList* list = NULL;
  if (requested.IsUndefined()) {
    list = JSLocaleList::New(ctx);
  } else {
    JSObject* req =
        requested.ToObject(ctx,
                           IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
    if (!req->IsClass<Class::LocaleList>()) {
      list = static_cast<JSLocaleList*>(requested.object());
    } else {
      list = detail_i18n::CreateLocaleList(
          ctx, req, IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
    }
  }

  bool best_fit = true;
  if (!options.IsUndefined()) {
    JSObject* opt = options.ToObject(
        ctx, IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
    const JSVal matcher =
        opt->Get(ctx, context::Intern(ctx, "localeMatcher"),
                 IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
    if (!matcher.IsUndefined()) {
      JSString* str = matcher.ToString(
          ctx,
          IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
      if (*str == *JSString::NewAsciiString(ctx, "lookup")) {
        best_fit = false;
      } else if (*str != *JSString::NewAsciiString(ctx, "best fit")) {
        e->Report(Error::Range,
                  "localeMatcher should be 'lookup' or 'best fit'");
        return core::i18n::LookupResult();
      }
    }
  }

  std::vector<std::string> locales;
  {
    const uint32_t len = internal::GetLength(
        ctx, list,
        IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
    for (uint32_t k = 0; k < len; ++k) {
      const JSVal res =
          list->Get(ctx, symbol::MakeSymbolFromIndex(k),
                    IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
      JSString* str = res.ToString(
          ctx,
          IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
      locales.push_back(str->GetUTF8());
    }
  }

  core::i18n::LookupResult res = (best_fit) ?
      core::i18n::BestFitMatch(it, last, locales.begin(), locales.end()) :
      core::i18n::LookupMatch(it, last, locales.begin(), locales.end());
  return res;
}

class Options {
 public:
  enum Type {
    BOOLEAN,
    STRING,
    NUMBER
  };

  struct Equaler {
   public:
    explicit Equaler(JSVal value) : target_(value) { }

    bool operator()(JSVal t) const {
      return JSVal::StrictEqual(t, target_);
    }
   private:
    const JSVal target_;
  };

  explicit Options(JSObject* options) : options_(options) { }

  JSVal Get(Context* ctx,
            Symbol property, Type type,
            JSVector* values, JSVal fallback, Error* e) {
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
        if (std::find_if(values->begin(),
                         values->end(), Equaler(value)) == values->end()) {
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
  typedef std::array<const char*, 4> Values;
  const char* key;
  std::size_t field;
  const char* property;
  Options::Type type;
  Values values;
};

typedef std::array<CollatorOption, 6> CollatorOptionTable;

static const CollatorOptionTable kCollatorOptionTable = { {
  { "kn", JSCollator::NUMERIC, "numeric",
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

class NumberOptions : public lv5::detail_i18n::Options {
 public:
  explicit NumberOptions(JSObject* options) : Options(options) { }

  int32_t GetNumber(Context* ctx,
                    Symbol property,
                    int32_t minimum,
                    int32_t maximum, int32_t fallback, Error* e) {
    const JSVal value =
        options()->Get(ctx, property, IV_LV5_ERROR_WITH(e, 0));
    if (!value.IsNullOrUndefined()) {
      const double res = value.ToNumber(ctx, IV_LV5_ERROR_WITH(e, 0));
      if (core::math::IsNaN(res) || res < minimum || res > maximum) {
        e->Report(Error::Range, "number option out of range");
        return 0;
      }
      return static_cast<int32_t>(std::floor(res));
    }
    return fallback;
  }
};

inline JSObject* ToDateTimeOptions(Context* ctx,
                                   JSVal op, bool date, bool time, Error* e) {
  JSObject* options = NULL;
  if (op.IsUndefined()) {
    options = JSObject::New(ctx);
  } else {
    options = op.ToObject(ctx, IV_LV5_ERROR_WITH(e, NULL));
  }

  // create Object that have options as [[Prototype]]
  {
    JSObject* tmp = JSObject::New(ctx);
    tmp->set_prototype(options);
    options = tmp;
  }

  bool need_default = true;

  typedef std::array<const char*, 4> DateProperties;
  static const DateProperties kDateProperties = { {
    "weekday", "year", "month", "day"
  } };
  if (date) {
    for (DateProperties::const_iterator it = kDateProperties.begin(),
         last = kDateProperties.end(); it != last; ++it) {
      const Symbol name = context::Intern(ctx, *it);
      const JSVal res = options->Get(ctx, name, IV_LV5_ERROR_WITH(e, NULL));
      if (!res.IsUndefined()) {
        need_default = false;
      }
    }
  }

  typedef std::array<const char*, 3> TimeProperties;
  static const TimeProperties kTimeProperties = { {
    "hour", "minute", "second"
  } };
  if (time) {
    for (TimeProperties::const_iterator it = kTimeProperties.begin(),
         last = kTimeProperties.end(); it != last; ++it) {
      const Symbol name = context::Intern(ctx, *it);
      const JSVal res = options->Get(ctx, name, IV_LV5_ERROR_WITH(e, NULL));
      if (!res.IsUndefined()) {
        need_default = false;
      }
    }
  }

  if (need_default && date) {
    typedef std::array<const char*, 3> DatePropertiesOnlyNumeric;
    static const DateProperties kDatePropertiesOnlyNumeric = { {
      "year", "month", "day"
    } };
    for (DatePropertiesOnlyNumeric::const_iterator
         it = kDatePropertiesOnlyNumeric.begin(),
         last = kDatePropertiesOnlyNumeric.end();
         it != last; ++it) {
      const Symbol name = context::Intern(ctx, *it);
      options->DefineOwnProperty(
          ctx,
          name,
          DataDescriptor(
              JSString::NewAsciiString(ctx, "numeric"),
              ATTR::W | ATTR::E | ATTR::C),
          true, IV_LV5_ERROR_WITH(e, NULL));
    }
  }

  if (need_default && date) {
    for (TimeProperties::const_iterator it = kTimeProperties.begin(),
         last = kTimeProperties.end(); it != last; ++it) {
      const Symbol name = context::Intern(ctx, *it);
      options->DefineOwnProperty(
          ctx,
          name,
          DataDescriptor(
              JSString::NewAsciiString(ctx, "numeric"),
              ATTR::W | ATTR::E | ATTR::C),
          true, IV_LV5_ERROR_WITH(e, NULL));
    }
  }

  return options;
}

struct DateTimeOption {
  typedef std::array<const char*, 5> Values;
  const char* key;
  JSDateTimeFormat::DateTimeFormatField field;
  Values values;
};
typedef std::array<DateTimeOption, 9> DateTimeOptions;
static const DateTimeOptions kDateTimeOptions = { {
  { "weekday",      JSDateTimeFormat::WEEKDAY,
    { { "narrow", "short", "long", NULL } } },
  { "era",          JSDateTimeFormat::ERA,
    { { "narrow", "short", "long", NULL } } },
  { "year",         JSDateTimeFormat::YEAR,
    { { "2-digit", "numeric", NULL } } },
  { "month",        JSDateTimeFormat::MONTH,
    { { "2-digit", "numeric", "narrow", "short", "long" } } },
  { "day",          JSDateTimeFormat::DAY,
    { { "2-digit", "numeric", NULL } } },
  { "hour",         JSDateTimeFormat::HOUR,
    { { "2-digit", "numeric", NULL } } },
  { "minute",       JSDateTimeFormat::MINUTE,
    { { "2-digit", "numeric", NULL } } },
  { "second",       JSDateTimeFormat::SECOND,
    { { "2-digit", "numeric", NULL } } },
  { "timeZoneName", JSDateTimeFormat::TIME_ZONE_NAME,
    { { "short", "long", NULL } } }
} };

inline JSObject* BasicFormatMatch(Context* ctx,
                                  JSObject* options,
                                  JSObject* formats, Error* e) {
  const int32_t removal_penalty = 120;
  const int32_t addition_penalty = 20;
  const int32_t long_less_penalty = 8;
  const int32_t long_more_penalty = 6;
  const int32_t short_less_penalty = 6;
  const int32_t short_more_penalty = 3;

  int32_t best_score = INT32_MIN;
  JSObject* best_format = NULL;
  const uint32_t len =
      internal::GetLength(ctx, formats, IV_LV5_ERROR_WITH(e, NULL));
  for (uint32_t i = 0; i < len; ++i) {
    const JSVal v =
        formats->Get(ctx, symbol::MakeSymbolFromIndex(i),
                     IV_LV5_ERROR_WITH(e, NULL));
    assert(v.IsObject());
    JSObject* format = v.object();
    int32_t score = 0;
    for (DateTimeOptions::const_iterator it = kDateTimeOptions.begin(),
         last = kDateTimeOptions.end(); it != last; ++it) {
      const Symbol name = context::Intern(ctx, it->key);
      const JSVal options_prop =
          options->Get(ctx, name, IV_LV5_ERROR_WITH(e, NULL));
      const PropertyDescriptor format_prop_desc =
          formats->GetOwnProperty(ctx, name);
      JSVal format_prop = JSUndefined;
      if (!format_prop_desc.IsEmpty()) {
        format_prop = formats->Get(ctx, name, IV_LV5_ERROR_WITH(e, NULL));
      }
      if (options_prop.IsUndefined() && !format_prop.IsUndefined()) {
        score -= addition_penalty;
      } else if (!options_prop.IsUndefined() && format_prop.IsUndefined()) {
        score -= removal_penalty;
      } else {
        typedef std::array<const char*, 5> ValueTypes;
        static const ValueTypes kValueTypes = { {
          "2-digit", "numeric", "narrow", "short", "long"
        } };
        int32_t options_prop_index = -1;
        if (options_prop.IsString()) {
          const std::string str = options_prop.string()->GetUTF8();
          const ValueTypes::const_iterator it =
              std::find(kValueTypes.begin(), kValueTypes.end(), str);
          if (it != kValueTypes.end()) {
            options_prop_index = std::distance(kValueTypes.begin(), it);
          }
        }
        int32_t format_prop_index = -1;
        if (format_prop.IsString()) {
          const std::string str = format_prop.string()->GetUTF8();
          const ValueTypes::const_iterator it =
              std::find(kValueTypes.begin(), kValueTypes.end(), str);
          if (it != kValueTypes.end()) {
            format_prop_index = std::distance(kValueTypes.begin(), it);
          }
        }
        const int32_t delta =
            (std::max)(
                (std::min)(format_prop_index - options_prop_index, 2), -2);
        switch (delta) {
          case 2: {
            score -= long_more_penalty;
            break;
          }
          case 1: {
            score -= short_more_penalty;
            break;
          }
          case -1: {
            score -= short_less_penalty;
            break;
          }
          case -2: {
            score -= long_less_penalty;
            break;
          }
        }
      }
    }
    if (score > best_score) {
      best_score = score;
      best_format = format;
    }
  }
  return best_format;
}

inline JSObject* BestFitFormatMatch(Context* ctx,
                                    JSObject* options,
                                    JSObject* formats, Error* e) {
  return BasicFormatMatch(ctx, options, formats, e);
}

}  // namespace detail_i18n

// initializers

inline JSVal JSCollator::Initialize(Context* ctx,
                                    JSCollator* obj,
                                    JSVal requested_locales,
                                    JSVal op,
                                    Error* e) {
  JSObject* options = NULL;
  if (op.IsUndefined()) {
    options = JSObject::New(ctx);
  } else {
    options = op.ToObject(ctx, IV_LV5_ERROR(e));
  }

  detail_i18n::Options opt(options);

  {
    JSVector* vec = JSVector::New(ctx);
    JSString* sort = JSString::NewAsciiString(ctx, "sort");
    JSString* search = JSString::NewAsciiString(ctx, "search");
    vec->push_back(sort);
    vec->push_back(search);
    const JSVal u = opt.Get(ctx,
                            context::Intern(ctx, "usage"),
                            detail_i18n::Options::STRING, vec,
                            sort, IV_LV5_ERROR(e));
    assert(u.IsString());
    obj->SetField(JSCollator::USAGE, u);
  }

  const core::i18n::LookupResult result =
      detail_i18n::ResolveLocale(
          ctx,
          ICUStringIteration(icu::NumberFormat::getAvailableLocales()),
          ICUStringIteration(),
          requested_locales,
          options,
          IV_LV5_ERROR(e));
  obj->SetField(JSCollator::LOCALE,
                JSString::NewAsciiString(ctx, result.locale()));
  icu::Locale locale(result.locale().c_str());
  {
    for (detail_i18n::CollatorOptionTable::const_iterator
         it = detail_i18n::kCollatorOptionTable.begin(),
         last = detail_i18n::kCollatorOptionTable.end();
         it != last; ++it) {
      JSVector* vec = NULL;
      if (it->values[0] != NULL) {
        vec = JSVector::New(ctx);
        for (detail_i18n::CollatorOption::Values::const_iterator
             oit = it->values.begin(),
             olast = it->values.end();
             oit != olast && *oit; ++oit) {
          vec->push_back(JSString::NewAsciiString(ctx, *oit));
        }
      }
      JSVal value = opt.Get(ctx,
                            context::Intern(ctx, it->property),
                            it->type, vec,
                            JSUndefined, IV_LV5_ERROR(e));
      if (it->type == detail_i18n::Options::BOOLEAN) {
        if (!value.IsUndefined()) {
          if (!value.IsBoolean()) {
            const JSVal str = value.ToString(ctx, IV_LV5_ERROR(e));
            value = JSVal::Bool(
                JSVal::StrictEqual(ctx->global_data()->string_true(), str));
          }
        } else {
          value = JSFalse;
        }
      }
      obj->SetField(it->field, value);
    }
  }

  {
    const detail_i18n::ExtensionMap map =
        detail_i18n::CreateExtensionMap(result.extensions());
    {
      const detail_i18n::ExtensionMap::const_iterator it = map.find("co");
      if (it != map.end()) {
        if (it->second.empty()) {
          obj->SetField(JSCollator::COLLATION,
                        JSString::NewAsciiString(ctx, "default"));
        } else {
          obj->SetField(JSCollator::COLLATION,
                        JSString::NewAsciiString(ctx, it->second));
        }
      }
    }
    for (detail_i18n::CollatorOptionTable::const_iterator
         it = detail_i18n::kCollatorOptionTable.begin(),
         last = detail_i18n::kCollatorOptionTable.end();
         it != last; ++it) {
      const JSVal option_value = obj->GetField(it->field);
      if (option_value.IsUndefined()) {
        const detail_i18n::ExtensionMap::const_iterator
            target = map.find(it->key);
        if (target != map.end()) {
          JSVal value;
          if (it->type == detail_i18n::Options::BOOLEAN) {
            obj->SetField(it->field,
                          JSVal::Bool(target->second == "true"));
          } else if (it->type == detail_i18n::Options::STRING) {
            if (it->values[0]) {
              for (const char* const * ptr = it->values.data(); *ptr; ++ptr) {
                if (*ptr == target->second) {
                  obj->SetField(
                      it->field,
                      JSString::NewAsciiString(ctx, target->second));
                }
              }
            }
          }
        }
      }
    }
  }

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
                      detail_i18n::Options::STRING, vec,
                      JSUndefined, IV_LV5_ERROR(e));
    if (s.IsUndefined()) {
      if (JSVal::StrictEqual(obj->GetField(JSCollator::USAGE),
                             JSString::NewAsciiString(ctx, "sort"))) {
        s = o_variant;
      } else {
        s = o_base;
      }
    }
    obj->SetField(JSCollator::SENSITIVITY, s);
  }

  {
    const JSVal ip = opt.Get(ctx,
                             context::Intern(ctx, "ignorePunctuation"),
                             detail_i18n::Options::BOOLEAN, NULL,
                             JSFalse, IV_LV5_ERROR(e));
    obj->SetField(JSCollator::IGNORE_PUNCTUATION, ip);
  }
  obj->SetField(JSCollator::BOUND_COMPARE, JSUndefined);

  // initialize icu::Collator
  detail_i18n::SetICUCollatorBooleanOption<
      JSCollator::NUMERIC,
      UCOL_NUMERIC_COLLATION>(obj, collator, IV_LV5_ERROR(e));
  detail_i18n::SetICUCollatorBooleanOption<
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
          ICUStringIteration(icu::NumberFormat::getAvailableLocales()),
          ICUStringIteration(),
          requested_locales,
          options,
          IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::LOCALE,
                JSString::NewAsciiString(ctx, result.locale()));
  icu::Locale locale(result.locale().c_str());
  {
    UErrorCode err = U_ZERO_ERROR;
    core::unique_ptr<icu::NumberingSystem> numbering_system(
        icu::NumberingSystem::createInstance(locale, err));
    if (U_FAILURE(err)) {
      e->Report(Error::Type, "numbering system initialization failed");
      return JSEmpty;
    }
    obj->SetField(JSNumberFormat::NUMBERING_SYSTEM,
                  JSString::NewAsciiString(ctx, numbering_system->getName()));
  }

  enum {
    DECIMAL_STYLE,
    PERCENT_STYLE,
    CURRENCY_STYLE
  } style = DECIMAL_STYLE;
  icu::DecimalFormat* format = NULL;
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

    UErrorCode status = U_ZERO_ERROR;
    if (JSVal::StrictEqual(t, currency)) {
      style = CURRENCY_STYLE;
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
#if IV_ICU_VERSION < 40700
        // old ICU
        const icu::NumberFormat::EStyle number_style =
            (JSVal::StrictEqual(cd, code)) ?
              icu::NumberFormat::kIsoCurrencyStyle :
            (JSVal::StrictEqual(cd, name)) ?
              icu::NumberFormat::kPluralCurrencyStyle :
              icu::NumberFormat::kCurrencyStyle;
#else
        const UNumberFormatStyle number_style =
            (JSVal::StrictEqual(cd, code)) ? UNUM_CURRENCY_ISO :
            (JSVal::StrictEqual(cd, name)) ? UNUM_CURRENCY_PLURAL :
            UNUM_CURRENCY;
#endif  // IV_ICU_VERSION < 40700
        format = static_cast<icu::DecimalFormat*>(
            icu::NumberFormat::createInstance(locale, number_style, status));
      }
    } else if (JSVal::StrictEqual(t, decimal)) {
      style = DECIMAL_STYLE;
      format = static_cast<icu::DecimalFormat*>(
          icu::NumberFormat::createInstance(locale, status));
    } else {
      style = PERCENT_STYLE;
      format = static_cast<icu::DecimalFormat*>(
          icu::NumberFormat::createPercentInstance(locale, status));
    }

    if (U_FAILURE(status)) {
      e->Report(Error::Type, "number format creation failed");
      return JSEmpty;
    }
    obj->set_number_format(format);
  }

  const int32_t minimum_integer_digits =
      opt.GetNumber(ctx,
                    context::Intern(ctx, "minimumIntegerDigits"),
                    1,
                    21,
                    1, IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::MINIMUM_INTEGER_DIGITS,
                JSVal::Int32(minimum_integer_digits));
  format->setMinimumIntegerDigits(minimum_integer_digits);

  // TODO(Constellation) use CurrencyDigits(c)
  const int32_t minimum_fraction_digits_default = 0;

  const int32_t minimum_fraction_digits =
      opt.GetNumber(ctx,
                    context::Intern(ctx, "minimumFractionDigits"),
                    0,
                    20,
                    minimum_fraction_digits_default, IV_LV5_ERROR(e));
  obj->SetField(JSNumberFormat::MINIMUM_FRACTION_DIGITS,
                JSVal::Int32(minimum_fraction_digits));
  format->setMinimumFractionDigits(minimum_fraction_digits);

  // TODO(Constellation) use CurrencyDigits(c)
  const int32_t maximum_fraction_digits_default =
      (style == CURRENCY_STYLE) ?
        (std::max)(minimum_fraction_digits_default, 3) :
      (style == PERCENT_STYLE) ?
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
  format->setMaximumFractionDigits(maximum_fraction_digits);

  if (options->HasProperty(ctx,
                           context::Intern(ctx, "minimumSignificantDigits")) ||
      options->HasProperty(ctx,
                           context::Intern(ctx, "maximumSignificantDigits"))) {
    const int32_t minimum_significant_digits =
        opt.GetNumber(ctx,
                      context::Intern(ctx, "minimumSignificantDigits"),
                      1,
                      21,
                      1, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::MINIMUM_SIGNIFICANT_DIGITS,
                  JSVal::Int32(minimum_significant_digits));
    format->setMinimumSignificantDigits(minimum_significant_digits);

    const int32_t maximum_significant_digits =
        opt.GetNumber(ctx,
                      context::Intern(ctx, "maximumSignificantDigits"),
                      minimum_significant_digits,
                      21,
                      21, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::MAXIMUM_SIGNIFICANT_DIGITS,
                  JSVal::Int32(maximum_significant_digits));
    format->setMaximumSignificantDigits(maximum_significant_digits);
  }

  {
    const JSVal ug = opt.Get(ctx,
                             context::Intern(ctx, "useGrouping"),
                             detail_i18n::Options::BOOLEAN, NULL,
                             JSTrue, IV_LV5_ERROR(e));
    obj->SetField(JSNumberFormat::USE_GROUPING, ug);
    format->setGroupingUsed(JSVal::StrictEqual(ug, JSTrue));
  }
  return obj;
}

inline JSVal JSDateTimeFormat::Initialize(Context* ctx,
                                          JSDateTimeFormat* obj,
                                          JSVal requested_locales,
                                          JSVal op,
                                          Error* e) {
  JSObject* options =
      detail_i18n::ToDateTimeOptions(ctx, op, true, false, IV_LV5_ERROR(e));

  detail_i18n::Options opt(options);

  int32_t num = 0;
  const icu::Locale* avail = icu::DateFormat::getAvailableLocales(num);
  std::vector<std::string> vec(num);
  for (int32_t i = 0; i < num; ++i) {
    vec[i] = avail[i].getName();
  }
  const core::i18n::LookupResult result =
      detail_i18n::ResolveLocale(
          ctx,
          vec.begin(),
          vec.end(),
          requested_locales,
          JSObject::New(ctx),
          IV_LV5_ERROR(e));
  obj->SetField(JSDateTimeFormat::LOCALE,
                JSString::NewAsciiString(ctx, result.locale()));
  icu::Locale locale(result.locale().c_str());
  {
    UErrorCode err = U_ZERO_ERROR;
    core::unique_ptr<icu::NumberingSystem> numbering_system(
        icu::NumberingSystem::createInstance(locale, err));
    if (U_FAILURE(err)) {
      e->Report(Error::Type, "numbering system initialization failed");
      return JSEmpty;
    }
    obj->SetField(JSDateTimeFormat::NUMBERING_SYSTEM,
                  JSString::NewAsciiString(ctx, numbering_system->getName()));
  }

  JSVal tz =
      options->Get(ctx, context::Intern(ctx, "timeZone"), IV_LV5_ERROR(e));
  if (!tz.IsUndefined()) {
    JSString* str = tz.ToString(ctx, IV_LV5_ERROR(e));
    std::vector<uint16_t> vec;
    for (JSString::const_iterator it = str->begin(),
         last = str->end(); it != last; ++it) {
      vec.push_back(core::i18n::ToLocaleIdentifierUpperCase(*it));
    }
    if (vec.size() != 3 ||
        vec[0] != 'U' || vec[1] != 'T' || vec[2] != 'C') {
      e->Report(Error::Range, "tz is not UTC");
      return JSEmpty;
    }
    obj->SetField(JSDateTimeFormat::TIME_ZONE, str);
  }

  {
    const JSVal hour12 = opt.Get(ctx,
                                 context::Intern(ctx, "hour12"),
                                 detail_i18n::Options::BOOLEAN, NULL,
                                 JSUndefined, IV_LV5_ERROR(e));
    obj->SetField(JSDateTimeFormat::HOUR12, hour12);
  }

  {
    for (detail_i18n::DateTimeOptions::const_iterator
         it = detail_i18n::kDateTimeOptions.begin(),
         last = detail_i18n::kDateTimeOptions.end();
         it != last; ++it) {
      const Symbol prop = context::Intern(ctx, it->key);
      JSVector* vec = JSVector::New(ctx);
      for (detail_i18n::DateTimeOption::Values::const_iterator
           dit = it->values.begin(),
           dlast = it->values.end();
           dit != dlast && *dit; ++dit) {
        vec->push_back(JSString::NewAsciiString(ctx, *dit));
      }
      const JSVal value = opt.Get(ctx,
                                  prop,
                                  detail_i18n::Options::STRING,
                                  vec,
                                  JSUndefined, IV_LV5_ERROR(e));
      obj->SetField(it->field, value);
    }
  }

  {
    JSString* basic = JSString::NewAsciiString(ctx, "basic");
    JSString* best_fit = JSString::NewAsciiString(ctx, "best fit");
    JSVector* vec = JSVector::New(ctx);
    vec->push_back(basic);
    vec->push_back(best_fit);
    const JSVal matcher = opt.Get(ctx,
                                  context::Intern(ctx, "formatMatch"),
                                  detail_i18n::Options::STRING,
                                  vec,
                                  best_fit, IV_LV5_ERROR(e));
    obj->SetField(JSDateTimeFormat::FORMAT_MATCH, matcher);
  }

  {
    const JSVal pattern =
        options->Get(ctx,
                     context::Intern(ctx, "pattern"),
                     IV_LV5_ERROR(e));
    obj->SetField(JSDateTimeFormat::PATTERN, pattern);
  }

  // Generate skeleton
  // See http://userguide.icu-project.org/formatparse/datetime
  std::string buffer;
  {
    // weekday
    const JSVal val= obj->GetField(JSDateTimeFormat::WEEKDAY);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "narrow") {
        buffer.append("EEEEE");
      } else if (target == "short") {
        buffer.push_back('E');
      } else {  // target == "long"
        buffer.append("EEEE");
      }
    }
  }
  {
    // era
    const JSVal val = obj->GetField(JSDateTimeFormat::ERA);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "narrow") {
        buffer.append("GGGGG");
      } else if (target == "short") {
        buffer.push_back('G');
      } else {  // target == "long"
        buffer.append("GGGG");
      }
    }
  }
  {
    // year
    const JSVal val = obj->GetField(JSDateTimeFormat::YEAR);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("yy");
      } else {  // target == "numeric"
        buffer.append("yyyy");
      }
    }
  }
  {
    // month
    const JSVal val = obj->GetField(JSDateTimeFormat::MONTH);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("MM");
      } else if (target == "numeric") {
        buffer.push_back('M');
      } else if (target == "narrow") {
        buffer.append("MMMMM");
      } else if (target == "short") {
        buffer.append("MMM");
      } else {  // target == "long"
        buffer.append("MMMM");
      }
    }
  }
  {
    // day
    const JSVal val = obj->GetField(JSDateTimeFormat::DAY);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("dd");
      } else {  // target == "numeric"
        buffer.append("d");
      }
    }
  }
  {
    // hour
    // check hour12 (AM / PM style or 24 style)
    const JSVal val = obj->GetField(JSDateTimeFormat::HOUR);
    const JSVal hour12 = obj->GetField(JSDateTimeFormat::HOUR12);
    if (!val.IsUndefined()) {
      const bool two_digit = val.string()->GetUTF8() == "2-digit";
      if (JSVal::StrictEqual(hour12, JSTrue)) {
        buffer.append(two_digit ? "hh" : "h");
      } else {
        buffer.append(two_digit ? "HH" : "H");
      }
    }
  }
  {
    // minute
    const JSVal val = obj->GetField(JSDateTimeFormat::MINUTE);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("mm");
      } else {  // target == "numeric"
        buffer.append("m");
      }
    }
  }
  {
    // second
    const JSVal val = obj->GetField(JSDateTimeFormat::SECOND);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "2-digit") {
        buffer.append("ss");
      } else {  // target == "numeric"
        buffer.append("s");
      }
    }
  }
  {
    // timezone name
    const JSVal val = obj->GetField(JSDateTimeFormat::TIME_ZONE_NAME);
    if (!val.IsUndefined()) {
      const std::string target = val.string()->GetUTF8();
      if (target == "short") {
        buffer.append("v");
      } else {  // target == "long"
        buffer.append("vvvv");
      }
    }
  }

  core::unique_ptr<icu::TimeZone> timezone;
  {
    // See http://userguide.icu-project.org/datetime/timezone
    const JSVal val = obj->GetField(JSDateTimeFormat::TIME_ZONE);
    if (val.IsUndefined()) {
      timezone.reset(icu::TimeZone::createDefault());
    } else {
      // already checked as UTC
      timezone.reset(icu::TimeZone::createTimeZone("GMT"));
    }
  }

  UErrorCode err = U_ZERO_ERROR;

  // See http://icu-project.org/apiref/icu4c/classCalendar.html
  // Calendar
  core::unique_ptr<icu::Calendar> calendar(
      icu::Calendar::createInstance(timezone.get(), locale, err));
  if (U_FAILURE(err)) {
    e->Report(Error::Type, "calendar failed");
    return JSEmpty;
  }
  timezone.release();  // release

  const icu::UnicodeString skeleton(buffer.c_str());
  core::unique_ptr<icu::DateTimePatternGenerator>
      generator(icu::DateTimePatternGenerator::createInstance(locale, err));
  if (U_FAILURE(err)) {
    e->Report(Error::Type, "pattern generator failed");
    return JSEmpty;
  }
  const icu::UnicodeString pattern = generator->getBestPattern(skeleton, err);
  if (U_FAILURE(err)) {
    e->Report(Error::Type, "pattern generator failed");
    return JSEmpty;
  }
  icu::SimpleDateFormat* format =
      new icu::SimpleDateFormat(pattern, locale, err);
  if (U_FAILURE(err)) {
    e->Report(Error::Type, "date formatter failed");
    return JSEmpty;
  }
  obj->set_date_time_format(format);

  format->adoptCalendar(calendar.get());
  obj->SetField(JSDateTimeFormat::CALENDAR,
                JSString::NewAsciiString(ctx, calendar->getType()));
  calendar.release();  // release
  return obj;
}

} }  // namespace iv::lv5
#endif  // IV_ENABLE_I18N
#endif  // IV_LV5_JSI18N_H_
