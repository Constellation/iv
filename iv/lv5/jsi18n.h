// Currently, we implement them by ICU.
// So this is enabled when IV_ENABLE_I18N flag is enabled and ICU is provided.
#ifndef IV_LV5_JSI18N_H_
#define IV_LV5_JSI18N_H_
#ifdef IV_ENABLE_I18N
#include <unicode/coll.h>
#include <iv/i18n.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class JSIntl : public JSObject {
 public:
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

  class CollatorHandle : public gc_cleanup {
   public:
    friend class JSCollator;
    explicit CollatorHandle(icu::Collator* coll) : collator(coll) { }
   private:
    core::unique_ptr<icu::Collator> collator;
  };


  enum CollatorField {
    USAGE = 0,
    LOCALE_MATCHER,
    BACKWARDS,
    CASE_LEVEL,
    NUMERIC,
    HIRAGANA_QUATERNARY,
    NORMALIZATION,
    CASE_FIRST,
    SENSITIVITY,
    IGNORE_PUNCTUATION,
    BOUND_COMPARE,
    NUM_OF_FIELDS
  };

  explicit JSCollator(Context* ctx)
    : JSObject(Map::NewUniqueMap(ctx)),
      collator_fields_(),
      handle_(new CollatorHandle(NULL)) {
  }

  JSCollator(Context* ctx, Map* map)
    : JSObject(map),
      collator_fields_(),
      handle_(new CollatorHandle(NULL)) {
  }

  static JSCollator* New(Context* ctx) {
    JSCollator* const collator = new JSCollator(ctx);
    collator->set_cls(JSCollator::GetClass());
    collator->set_prototype(
        context::GetClassSlot(ctx, Class::Collator).prototype);
    return collator;
  }

  JSVal GetCollatorField(std::size_t n) {
    assert(n < collator_fields_.size());
    return collator_fields_[n];
  }

  void SetCollatorField(std::size_t n, JSVal v) {
    assert(n < collator_fields_.size());
    collator_fields_[n] = v;
  }

  void set_collator(icu::Collator* collator) {
    handle_->collator.reset(collator);
  }

  icu::Collator* collator() const { return handle_->collator.get(); }

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
  CollatorHandle* handle_;
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
};

class JSDateTimeFormat : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(DateTimeFormat)
};

} }  // namespace iv::lv5
#endif  // IV_ENABLE_I18N
#endif  // IV_LV5_JSI18N_H_
