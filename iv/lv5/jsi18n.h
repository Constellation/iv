// Currently, we implement them by ICU.
// So this is enabled when IV_ENABLE_I18N flag is enabled and ICU is provided.
#ifndef IV_LV5_JSI18N_H_
#define IV_LV5_JSI18N_H_
#ifdef IV_ENABLE_I18N
#include <iterator>
#include <unicode/coll.h>
#include <unicode/decimfmt.h>
#include <unicode/numsys.h>
#include <iv/i18n.h>
#include <iv/detail/unique_ptr.h>
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

  void MakeInitializedLocaleList() {
  }
};

class JSCollator : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(Collator)

  enum CollatorField {
    USAGE = 0,
    BACKWARDS,
    CASE_LEVEL,
    NUMERIC,
    HIRAGANA_QUATERNARY,
    NORMALIZATION,
    CASE_FIRST,
    SENSITIVITY,
    IGNORE_PUNCTUATION,
    BOUND_COMPARE,
    LOCALE,
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
      number_format_(new JSIntl::GCHandle<icu::NumberFormat>()),
      fields_() {
  }

  JSNumberFormat(Context* ctx, Map* map)
    : JSObject(map),
      number_format_(new JSIntl::GCHandle<icu::NumberFormat>()),
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

  void set_number_format(icu::NumberFormat* number_format) {
    number_format_->handle.reset(number_format);
  }

  icu::NumberFormat* number_format() const {
    return number_format_->handle.get();
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
  JSIntl::GCHandle<icu::NumberFormat>* number_format_;
  std::array<JSVal, NUM_OF_FIELDS> fields_;
};

class JSDateTimeFormat : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(DateTimeFormat)
};

namespace detail_i18n {

inline JSLocaleList* CreateLocaleList(
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

template<typename AvailIter>
inline core::i18n::LookupResult ResolveLocale(Context* ctx,
                                              AvailIter it,
                                              AvailIter last,
                                              JSVal requested,
                                              JSVal options, Error* e) {
  JSLocaleList* list = NULL;
  JSObject* req =
      requested.ToObject(ctx, IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
  if (!req->IsClass<Class::LocaleList>()) {
    list = static_cast<JSLocaleList*>(requested.object());
  } else {
    list = detail_i18n::CreateLocaleList(
        ctx, req, IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
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

  return (best_fit) ?
      core::i18n::BestFitMatch(it, last, locales.begin(), locales.end()) :
      core::i18n::LookupMatch(it, last, locales.begin(), locales.end());
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
        e->Report(Error::Range, "option out of range");
        return 0;
      }
      return static_cast<int32_t>(std::floor(res));
    }
    return fallback;
  }
};

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
                            detail_i18n::Options::STRING, vec->ToJSArray(),
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
  // TODO(Constellation) not implement extension check
  icu::Locale locale(result.locale().c_str());

  {
    for (detail_i18n::CollatorOptionTable::const_iterator
         it = detail_i18n::kCollatorOptionTable.begin(),
         last = detail_i18n::kCollatorOptionTable.end();
         it != last; ++it) {
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
      if (it->type == detail_i18n::Options::BOOLEAN) {
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
                      detail_i18n::Options::STRING, vec->ToJSArray(),
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
      JSCollator::BACKWARDS,
      UCOL_FRENCH_COLLATION>(obj, collator, IV_LV5_ERROR(e));
  detail_i18n::SetICUCollatorBooleanOption<
      JSCollator::CASE_LEVEL,
      UCOL_CASE_LEVEL>(obj, collator, IV_LV5_ERROR(e));
  detail_i18n::SetICUCollatorBooleanOption<
      JSCollator::NUMERIC,
      UCOL_NUMERIC_COLLATION>(obj, collator, IV_LV5_ERROR(e));
  detail_i18n::SetICUCollatorBooleanOption<
      JSCollator::HIRAGANA_QUATERNARY,
      UCOL_HIRAGANA_QUATERNARY_MODE>(obj, collator, IV_LV5_ERROR(e));
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
  // TODO(Constellation) not implement extension check
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
                            detail_i18n::Options::STRING, vec->ToJSArray(),
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
                                 detail_i18n::Options::STRING, vec->ToJSArray(),
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


} }  // namespace iv::lv5
#endif  // IV_ENABLE_I18N
#endif  // IV_LV5_JSI18N_H_
