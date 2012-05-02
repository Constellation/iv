// Currently, we implement them by ICU.
// So this is enabled when IV_ENABLE_I18N flag is enabled and ICU is provided.
#ifndef IV_LV5_JSI18N_H_
#define IV_LV5_JSI18N_H_
#ifdef IV_ENABLE_I18N
#include <iterator>
#include <unicode/coll.h>
#include <unicode/rbnf.h>
#include <iv/i18n.h>
#include <iv/detail/unique_ptr.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
#include <iv/detail/unique_ptr.h>
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
    // TODO(Constellation): fix default plain internal options
    JSCollator* obj = new JSCollator(ctx, map);
    const icu::Locale locale = icu::Locale::getDefault();
    UErrorCode status = U_ZERO_ERROR;
    icu::Collator* collator = icu::Collator::createInstance(locale, status);
    obj->set_collator(collator);
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
    // TODO(Constellation): fix default plain internal options
    JSNumberFormat* obj = new JSNumberFormat(ctx, map);
    const icu::Locale locale = icu::Locale::getDefault();
    UErrorCode status = U_ZERO_ERROR;
    icu::NumberFormat* format =
        icu::NumberFormat::createInstance(locale, status);
    obj->set_number_format(format);
    return obj;
  }

 private:
  JSIntl::GCHandle<icu::NumberFormat>* number_format_;
  std::array<JSVal, NUM_OF_FIELDS> fields_;
};

class JSDateTimeFormat : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(DateTimeFormat)
};

} }  // namespace iv::lv5
#endif  // IV_ENABLE_I18N
#endif  // IV_LV5_JSI18N_H_
