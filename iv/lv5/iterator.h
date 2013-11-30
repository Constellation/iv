#ifndef IV_LV5_ITERATOR_H_
#define IV_LV5_ITERATOR_H_
#include <iv/lv5/context.h>
#include <iv/lv5/error.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/jsval.h>
namespace iv {
namespace lv5 {

class Iterator {
 public:
  static JSVal Step(Context* ctx, JSVal iterator, JSVal value, Error* e);
  static JSObject* Next(Context* ctx, JSVal iterator, JSVal value, Error* e);
  static JSVal Value(Context* ctx, JSObject* result, Error* e);
  static bool Complete(Context* ctx, JSObject* result, Error* e);
};

inline JSVal Iterator::Step(Context* ctx,
                            JSVal iterator, JSVal value, Error* e) {
  JSObject* result = Next(ctx, iterator, value, IV_LV5_ERROR(e));
  const bool done = Complete(ctx, result, IV_LV5_ERROR(e));
  if (done) {
    return JSFalse;
  }
  return result;
}

inline JSObject* Iterator::Next(Context* ctx,
                                JSVal iterator, JSVal value, Error* e) {
  const JSVal result =
      internal::Invoke(
          ctx, iterator, symbol::next(), { value }, IV_LV5_ERROR(e));
  if (result.IsObject()) {
    e->Report(Error::Type, "result of iterator.next call is not object");
    return nullptr;
  }
  return result.object();
}

inline JSVal Iterator::Value(Context* ctx, JSObject* result, Error* e) {
  return result->Get(ctx, symbol::value(), IV_LV5_ERROR(e));
}

inline bool Iterator::Complete(Context* ctx, JSObject* result, Error* e) {
  const JSVal done = result->Get(ctx, symbol::done(), IV_LV5_ERROR(e));
  return done.ToBoolean();
}

} }  // namespace iv::lv5
#endif  // IV_LV5_ITERATOR_H_
