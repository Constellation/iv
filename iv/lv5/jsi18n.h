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
namespace iv {
namespace lv5 {

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

  // i18n draft CreateLocaleList abstract operation
  static JSLocaleList* CreateLocaleList(Context* ctx, JSVal target, Error* e);
};

inline JSLocaleList* JSLocaleList::CreateLocaleList(Context* ctx,
                                                    JSVal target, Error* e) {
  std::vector<std::string> list;
  if (target.IsUndefined()) {
    list.push_back(ctx->i18n()->DefaultLocale());
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

namespace detail_i18n {

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
      const AvailIter t = ctx->i18n()->IndexOfMatch(it, last, locale);
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
    list = JSLocaleList::CreateLocaleList(ctx, req, IV_LV5_ERROR(e));
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
      list = JSLocaleList::CreateLocaleList(
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
      ctx->i18n()->BestFitMatch(it, last, locales.begin(), locales.end()) :
      ctx->i18n()->LookupMatch(it, last, locales.begin(), locales.end());
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

}  // namespace detail_i18n
} }  // namespace iv::lv5
#ifdef IV_ENABLE_I18N
#include <iv/lv5/jsi18n_icu.h>
#else
#include <iv/lv5/jsi18n_iv.h>
#endif  // IV_ENABLE_I18N
#endif  // IV_LV5_JSI18N_H_
