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
  typedef core::i18n::LanguageTagScanner<JSString::const_iterator> Scanner;
  std::vector<std::string> list;
  const JSVal target = args.At(0);
  Context* ctx = args.ctx();
  if (target.IsUndefined()) {
    // TODO(Constellation) implement default locale system
    list.push_back("en-US");
  } else {
    JSObject* obj = target.ToObject(ctx, IV_LV5_ERROR(e));
    const uint32_t len = internal::GetLength(ctx, obj, IV_LV5_ERROR(e));
    for (uint32_t k = 0; k < len; ++k) {
      const Symbol name = symbol::MakeSymbolFromIndex(k);
      if (obj->HasProperty(ctx, name)) {
        const JSVal value = obj->Get(ctx, name, IV_LV5_ERROR(e));
        if (!(value.IsString() || value.IsObject())) {
          e->Report(Error::Type, "locale should be string or object");
          return JSEmpty;
        }
        JSString* tag = value.ToString(ctx, IV_LV5_ERROR(e));
        Scanner scanner(tag->begin(), tag->end());
        if (!scanner.IsWellFormed()) {
          e->Report(Error::Range, "locale pattern is not well formed");
          return JSEmpty;
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
    JSVal value = options_->Get(ctx, property, IV_LV5_ERROR(e));
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
  const bool res = obj->GetCollatorField(TYPE).ToBoolean();
  collator->setAttribute(ATTR, res ? UCOL_ON : UCOL_OFF, status);
  if (U_FAILURE(status)) {
    e->Report(Error::Type, "icu collator initialization failed");
    return;
  }
}

// TODO(Constellation) not implemented yet
inline JSVal CollatorConstructor(const Arguments& args, Error* e) {
  const JSVal arg1 = args.At(0);
  const JSVal arg2 = args.At(1);
  Context* ctx = args.ctx();
  JSCollator* obj = JSCollator::New(ctx);

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
    obj->SetCollatorField(JSCollator::USAGE, u);
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
    obj->SetCollatorField(JSCollator::LOCALE_MATCHER, matcher);
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
      obj->SetCollatorField(it->field, value);
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
      if (JSVal::StrictEqual(obj->GetCollatorField(JSCollator::USAGE),
                             JSString::NewAsciiString(ctx, "sort"))) {
        s = o_variant;
      } else {
        // TODO(Constellation) dataLocale
        s = o_base;
      }
    }
    obj->SetCollatorField(JSCollator::SENSITIVITY, s);
  }

  {
    const JSVal ip = opt.Get(ctx,
                             context::Intern(ctx, "ignorePunctuation"),
                             Options::BOOLEAN, NULL,
                             JSFalse, IV_LV5_ERROR(e));
    obj->SetCollatorField(JSCollator::IGNORE_PUNCTUATION, ip);
  }
  obj->SetCollatorField(JSCollator::BOUND_COMPARE, JSUndefined);

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
    const JSVal v = obj->GetCollatorField(JSCollator::CASE_FIRST);
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
    const JSVal v = obj->GetCollatorField(JSCollator::SENSITIVITY);
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
    if (obj->GetCollatorField(JSCollator::IGNORE_PUNCTUATION).ToBoolean()) {
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
  Context* ctx = args.ctx();
  JSObject* o = args.this_binding().ToObject(ctx, IV_LV5_ERROR(e));
  if (!o->IsClass<Class::Collator>()) {
    e->Report(Error::Type,
              "Intl.Collator.prototype.compare is not generic getter");
    return JSEmpty;
  }
  JSCollator* obj = static_cast<JSCollator*>(o);
  JSVal bound = obj->GetCollatorField(JSCollator::BOUND_COMPARE);
  if (bound.IsUndefined()) {
    bound = JSCollatorBoundFunction::New(ctx, obj);
    obj->SetCollatorField(JSCollator::BOUND_COMPARE, bound);
  }
  return bound;
}

inline JSVal CollatorResolvedOptionsGetter(const Arguments& args, Error* e) {
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
           collator->GetCollatorField(JSCollator::USAGE),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "sensitivity"),
           collator->GetCollatorField(JSCollator::SENSITIVITY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "backwards"),
           collator->GetCollatorField(JSCollator::BACKWARDS),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "caseLevel"),
           collator->GetCollatorField(JSCollator::CASE_LEVEL),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "numeric"),
           collator->GetCollatorField(JSCollator::NUMERIC),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "hiraganaQuaternary"),
           collator->GetCollatorField(JSCollator::HIRAGANA_QUATERNARY),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "normalization"),
           collator->GetCollatorField(JSCollator::NORMALIZATION),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "caseFirst"),
           collator->GetCollatorField(JSCollator::CASE_FIRST),
           ATTR::W | ATTR::E | ATTR::C)
      .def(context::Intern(ctx, "ignorePunctuation"),
           collator->GetCollatorField(JSCollator::IGNORE_PUNCTUATION),
           ATTR::W | ATTR::E | ATTR::C);
  return obj;
}

#undef IV_LV5_I18N_INTL_CHECK
} } }  // namespace iv::lv5::runtime
#endif  // IV_ENABLE_I18N_H_
#endif  // IV_LV5_RUNTIME_I18N_H_
