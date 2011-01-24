#ifndef _IV_LV5_JSREGEXP_H_
#define _IV_LV5_JSREGEXP_H_
#include "jsobject.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "jsregexp_impl.h"
namespace iv {
namespace lv5 {
class Context;
class JSRegExp : public JSObject {
 public:
  JSRegExp(Context* ctx,
           const core::UStringPiece& value,
           const core::UStringPiece& flags);

  JSRegExp(Context* ctx,
           const core::UStringPiece& value,
           const JSRegExpImpl* reg);

  explicit JSRegExp(Context* ctx);

  inline bool IsValid() const {
    return impl_->IsValid();
  }

  JSString* source(Context* ctx);

  static JSRegExp* New(Context* ctx);

  static JSRegExp* New(Context* ctx,
                       const core::UStringPiece& value,
                       const JSRegExpImpl* reg);

  static JSRegExp* New(Context* ctx,
                       const core::UStringPiece& value,
                       const core::UStringPiece& flags);

  static JSRegExp* New(Context* ctx, JSRegExp* reg);

  static JSRegExp* NewPlain(Context* ctx);

  bool global() const {
    return impl_->global();
  }

  bool ignore() const {
    return impl_->ignore();
  }

  bool multiline() const {
    return impl_->multiline();
  }

  int LastIndex(Context* ctx, Error* e);
  void SetLastIndex(Context* ctx, int i, Error* e);

  JSVal ExecuteOnce(Context* ctx,
                    const core::UStringPiece& piece,
                    int previous_index,
                    std::vector<int>* offset_vector,
                    Error* e);

  JSVal Execute(Context* ctx,
                const core::UStringPiece& piece,
                Error* e);

  JSVal ExecuteGlobal(Context* ctx,
                      const core::UStringPiece& piece,
                      Error* e);

  JSVal ExecGlobal(Context* ctx, JSString* str, Error* e);
  JSVal Exec(Context* ctx, JSString* str, Error* e);

 private:
  void InitializeProperty(Context* ctx);

  const JSRegExpImpl* impl() const {
    return impl_;
  }

  const JSRegExpImpl* impl_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREGEXP_H_
