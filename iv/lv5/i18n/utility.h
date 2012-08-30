#ifndef IV_LV5_I18N_UTILITY_H_
#define IV_LV5_I18N_UTILITY_H_
#include <iv/detail/unique_ptr.h>
namespace iv {
namespace lv5 {
namespace i18n {

template<typename T>
class GCHandle : public gc_cleanup {
 public:
  core::unique_ptr<T> handle;
};


inline JSVector* CanonicalizeLocaleList(Context* ctx, JSVal locales, Error* e) {
  if (locales.IsUndefined()) {
    return JSVector::New(ctx);
  }

  JSVector* seen = JSVector::New(ctx);
  std::unordered_set<std::string> checker;
  JSObject* obj = locales.ToObject(ctx, IV_LV5_ERROR_WITH(e, NULL));
  const uint32_t len =
      internal::GetLength(ctx, obj, IV_LV5_ERROR_WITH(e, NULL));
  for (uint32_t k = 0; k < len; ++k) {
    const Symbol pk = symbol::MakeSymbolFromIndex(k);
    if (obj->HasProperty(ctx, pk)) {
      const JSVal value = obj->Get(ctx, pk, IV_LV5_ERROR_WITH(e, NULL));
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
      if (checker.find(canonicalized) == checker.end()) {
        checker.insert(canonicalized);
        JSString* str =
            JSString::NewAsciiString(ctx, canonicalized, IV_LV5_ERROR_WITH(e, NULL));
        seen->push_back(str);
      }
    }
  }
  return seen;
}

class Options {
 public:
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

  template<typename Iter>
  JSString* GetString(Context* ctx,
                      Symbol property,
                      Iter it,
                      Iter last, const char* fallback, Error* e) {
    JSVal value = options()->Get(ctx, property, IV_LV5_ERROR_WITH(e, NULL));
    if (!value.IsUndefined()) {
      JSString* str = value.ToString(ctx, IV_LV5_ERROR_WITH(e, NULL));
      if (it != last) {
        bool found = false;
        for (; it != last; ++it) {
          if (str->compare(*it) == 0) {
            found = true;
            break;
          }
        }
        if (!found) {
          e->Report(Error::Range, "option out of range");
          return NULL;
        }
        return str;
      }
      return str;
    }
    if (fallback) {
      return JSString::NewAsciiString(ctx, fallback, e);
    }
    return NULL;
  }

  bool GetBoolean(Context* ctx, Symbol property, bool fallback, Error* e) {
    JSVal value = options()->Get(ctx, property, IV_LV5_ERROR_WITH(e, false));
    if (!value.IsUndefined()) {
      return value.ToBoolean();
    }
    return fallback;
  }

  JSObject* options() { return options_; }

 private:
  JSObject* options_;
};

class NumberOptions : public Options {
 public:
  explicit NumberOptions(JSObject* options) : Options(options) { }

  int32_t GetNumber(Context* ctx,
                    Symbol property,
                    int32_t minimum,
                    int32_t maximum, int32_t fallback, Error* e) {
    const JSVal value =
        options()->Get(ctx, property, IV_LV5_ERROR_WITH(e, 0));
    if (!value.IsUndefined()) {
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
        opt->Get(ctx, context::Intern(ctx, "localeMatcher"),
                 IV_LV5_ERROR_WITH(e, NULL));
    if (!matcher.IsUndefined()) {
      JSString* str = matcher.ToString(ctx, IV_LV5_ERROR_WITH(e, NULL));
      if (str->compare("lookup") == 0) {
        best_fit = false;
      } else if (str->compare("best fit") != 0) {
        e->Report(Error::Range, "lookupMatcher pattern is not known");
        return NULL;
      }
    }
  }

  JSVector* subset =
      (best_fit)
      ? BestFitSupportedLocales(ctx, it, last, requested, e)
      : LookupSupportedLocales(ctx, it, last, requested, e);
  IV_LV5_ERROR_GUARD_WITH(e, NULL);

  JSArray* result = subset->ToJSArray();
  PropertyNamesCollector collector;
  result->GetOwnPropertyNames(ctx, &collector,
                              JSObject::INCLUDE_NOT_ENUMERABLE);
  for (PropertyNamesCollector::Names::const_iterator
       it = collector.names().begin(),
       last = collector.names().end();
       it != last; ++it) {
    const Symbol sym = *it;
    PropertyDescriptor desc = result->GetOwnProperty(ctx, sym);
    assert(desc.IsData());
    desc.AsDataDescriptor()->set_writable(false);
    desc.set_configurable(false);
    result->DefineOwnProperty(ctx, sym, desc, true, IV_LV5_ERROR_WITH(e, NULL));
  }

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
        opt->Get(ctx, context::Intern(ctx, "localeMatcher"),
                 IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
    if (!matcher.IsUndefined()) {
      JSString* str = matcher.ToString(
          ctx,
          IV_LV5_ERROR_WITH(e, core::i18n::LookupResult()));
      if (str->compare("lookup") == 0) {
        best_fit = false;
      } else if (str->compare("best fit") != 0) {
        e->Report(Error::Range, "lookupMatcher pattern is not known");
        return core::i18n::LookupResult();
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
    static const DatePropertiesOnlyNumeric kDatePropertiesOnlyNumeric = { {
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
              JSString::NewAsciiString(ctx, "numeric", e),
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
              JSString::NewAsciiString(ctx, "numeric", e),
              ATTR::W | ATTR::E | ATTR::C),
          true, IV_LV5_ERROR_WITH(e, NULL));
    }
  }

  return options;
}

} } }  // namespace iv::lv5::i18n
#endif  // IV_LV5_I18N_UTILITY_H_
