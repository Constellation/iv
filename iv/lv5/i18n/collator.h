#ifndef IV_LV5_I18N_COLLATOR_H_
#define IV_LV5_I18N_COLLATOR_H_
#include <iv/i18n.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/bind.h>
#include <iv/lv5/context_fwd.h>
#include <iv/lv5/i18n/utility.h>
namespace iv {
namespace lv5 {
namespace i18n {

// 10.1.1.1 InitializeCollator(collator, locales, options)
inline JSObject* InitializeCollator(Context* ctx,
                                    JSObject* collator,
                                    JSVal locales, JSVal op, Error* e) {
  if (collator->HasOwnProperty(
          ctx, ctx->i18n()->symbols().initializedIntlObject())) {
    e->Report(Error::Type,
              "object has been already initialized as Intl group object");
    return nullptr;
  }

  collator->DefineOwnProperty(
      ctx,
      ctx->i18n()->symbols().initializedIntlObject(),
      DataDescriptor(JSTrue, ATTR::N),
      false, IV_LV5_ERROR(e));

  JSVector* requested_locales =
      CanonicalizeLocaleList(ctx, locales, IV_LV5_ERROR(e));

  JSObject* o = nullptr;
  if (op.IsUndefined()) {
    o = JSObject::New(ctx);
  } else {
    o = op.ToObject(ctx, IV_LV5_ERROR(e));
  }

  Options options(o);

  static const std::array<core::StringPiece, 2> k6 = { {
    "sort",
    "search"
  } };
  JSString* u =
      options.GetString(
          ctx, symbol::usage(), k6.begin(), k6.end(), "sort", IV_LV5_ERROR(e));

  collator->DefineOwnProperty(
      ctx,
      ctx->i18n()->symbols().usage(),
      DataDescriptor(u, ATTR::N),
      false, IV_LV5_ERROR(e));

  // TODO(Constellation) collator constructor step 8

  if (u->compare("sort") == 0) {
  }

  JSObject* opt = JSObject::New(ctx);
  return collator;
}

} } }  // namespace iv::lv5::i18n
#endif  // IV_LV5_I18N_COLLATOR_H_
