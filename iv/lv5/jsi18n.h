#ifndef IV_LV5_JSI18N_H_
#define IV_LV5_JSI18N_H_
#include <iterator>
#include <string>
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
#include <iv/lv5/runtime_fwd.h>
#include <iv/lv5/jsvector.h>
#include <iv/lv5/i18n/utility.h>
namespace iv {
namespace lv5 {

namespace detail_i18n {

template<typename AvailIter>
inline JSVector* LookupSupportedLocales(Context* ctx,
                                        AvailIter it, AvailIter last,
                                        JSVector* requested, Error* e) {
  JSVector* subset = JSVector::New(ctx);
  for (JSVector::const_iterator i = requested->begin(),
       iz = requested->end(); i != iz; ++i) {
    const JSVal res = *i;
    JSString* str = res.ToString(ctx, IV_LV5_ERROR_WITH(e, NULL));
    const std::string locale(
        core::i18n::LanguageTagScanner::RemoveExtension(str->begin(),
                                                        str->end()));
    const AvailIter t = ctx->i18n()->BestAvailableLocale(it, last, locale);
    if (t != last) {
      JSString* str =
          JSString::NewAsciiString(ctx, locale, IV_LV5_ERROR_WITH(e, NULL));
      subset->push_back(str);
    }
  }
  return subset;
}

template<typename AvailIter>
inline JSVector* BestFitSupportedLocales(Context* ctx,
                                         AvailIter it, AvailIter last,
                                         JSVector* requested, Error* e) {
  return LookupSupportedLocales(ctx, it, last, requested, e);
}

template<typename AvailIter>
inline JSArray* SupportedLocales(Context* ctx,
                                 AvailIter it, AvailIter last,
                                 JSVector* requested, JSVal options, Error* e) {
  bool best_fit = true;
  if (!options.IsUndefined()) {
    JSObject* opt = options.ToObject(ctx, IV_LV5_ERROR_WITH(e, NULL));
    const JSVal matcher =
        opt->Get(ctx, ctx->Intern("localeMatcher"),
                 IV_LV5_ERROR_WITH(e, NULL));
    if (!matcher.IsUndefined()) {
      JSString* str = matcher.ToString(ctx, IV_LV5_ERROR_WITH(e, NULL));
      const std::string res = str->GetUTF8();
      if (res == "lookup") {
        best_fit = false;
      }
    }
  }

  JSVector* subset =
      (best_fit)
      ? BestFitSupportedLocales(ctx, it, last, requested, e)
      : LookupSupportedLocales(ctx, it, last, requested, e);
  IV_LV5_ERROR_GUARD_WITH(e, NULL);

  JSArray* result = subset->ToJSArray();
  ScopedArguments arguments(ctx, 1, IV_LV5_ERROR_WITH(e, NULL));
  arguments[0] = result;
  runtime::ObjectFreeze(arguments, IV_LV5_ERROR_WITH(e, NULL));
  result->set_extensible(true);
  return result;
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
                                              JSVector* requested,
                                              JSVal options, Error* e) {
  bool best_fit = true;
  if (!options.IsUndefined()) {
    JSObject* opt = options.ToObject(
        ctx, IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
    const JSVal matcher =
        opt->Get(ctx, ctx->Intern("localeMatcher"),
                 IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
    if (!matcher.IsUndefined()) {
      JSString* str = matcher.ToString(
          ctx,
          IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
      const std::string res = str->GetUTF8();
      if (res == "lookup") {
        best_fit = false;
      }
    }
  }

  std::vector<std::string> locales;
  {
    for (JSVector::const_iterator it = requested->begin(),
         last = requested->end(); it != last; ++it) {
      JSString* str = it->ToString(
          ctx,
          IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
      locales.push_back(str->GetUTF8());
    }
  }
  core::i18n::LookupResult res = (best_fit) ?
      ctx->i18n()->BestFitMatcher(it, last, locales.begin(), locales.end()) :
      ctx->i18n()->LookupMatcher(it, last, locales.begin(), locales.end());
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
    tmp->ChangePrototype(ctx, options);
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
      const Symbol name = ctx->Intern(*it);
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
      const Symbol name = ctx->Intern(*it);
      const JSVal res = options->Get(ctx, name, IV_LV5_ERROR_WITH(e, NULL));
      if (!res.IsUndefined()) {
        need_default = false;
      }
    }
  }

  if (need_default && date) {
    typedef std::array<const char*, 3> DatePropertiesOnlyNumeric;
    static const DatePropertiesOnlyNumeric kDatePropertiesOnlyNumeric = { {
      "year", "month", "day"
    } };
    for (DatePropertiesOnlyNumeric::const_iterator
         it = kDatePropertiesOnlyNumeric.begin(),
         last = kDatePropertiesOnlyNumeric.end();
         it != last; ++it) {
      const Symbol name = ctx->Intern(*it);
      options->DefineOwnProperty(
          ctx,
          name,
          DataDescriptor(
              JSString::NewAsciiString(ctx, "numeric", e),
              ATTR::W | ATTR::E | ATTR::C),
          true, IV_LV5_ERROR_WITH(e, NULL));
    }
  }

  if (need_default && date) {
    for (TimeProperties::const_iterator it = kTimeProperties.begin(),
         last = kTimeProperties.end(); it != last; ++it) {
      const Symbol name = ctx->Intern(*it);
      options->DefineOwnProperty(
          ctx,
          name,
          DataDescriptor(
              JSString::NewAsciiString(ctx, "numeric", e),
              ATTR::W | ATTR::E | ATTR::C),
          true, IV_LV5_ERROR_WITH(e, NULL));
    }
  }

  return options;
}

}  // namespace detail_i18n
} }  // namespace iv::lv5

#include <iv/lv5/i18n/i18n.h>
#ifdef IV_ENABLE_I18N
#include <iv/lv5/jsi18n_icu.h>
#endif  // IV_ENABLE_I18N
#endif  // IV_LV5_JSI18N_H_
