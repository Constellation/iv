#ifndef _IV_LV5_JSREGEXP_H_
#define _IV_LV5_JSREGEXP_H_
#include <vector>
#include <utility>
#include "stringpiece.h"
#include "ustringpiece.h"
#include "lv5/jsobject.h"
#include "lv5/jsregexp_impl.h"
#include "lv5/matchresult.h"
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

  JSVal ExecGlobal(Context* ctx, JSString* str, Error* e);
  JSVal Exec(Context* ctx, JSString* str, Error* e);

  template<typename String>
  regexp::MatchResult Match(const String& str,
                            int index,
                            regexp::PairVector* result) const {
    const uint32_t num_of_captures = impl_->number_of_captures();
    std::vector<int> offset_vector((num_of_captures + 1) * 3, -1);
    const int rc = impl_->ExecuteOnce(str,
                                      index, &offset_vector);
    if (rc == jscre::JSRegExpErrorNoMatch ||
        rc == jscre::JSRegExpErrorHitLimit) {
      return std::tr1::make_tuple(0, 0, false);
    }
    if (rc < 0) {
      return std::tr1::make_tuple(0, 0, false);
    }
    for (int i = 1, len = num_of_captures + 1; i < len; ++i) {
      result->push_back(std::make_pair(offset_vector[i*2], offset_vector[i*2+1]));
    }
    return std::tr1::make_tuple(offset_vector[0], offset_vector[1], true);
  }

 private:
  void InitializeProperty(Context* ctx);

  const JSRegExpImpl* impl() const {
    return impl_;
  }

  const JSRegExpImpl* impl_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREGEXP_H_
