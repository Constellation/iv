#ifndef _IV_LV5_JSREGEXP_H_
#define _IV_LV5_JSREGEXP_H_
#include <vector>
#include <utility>
#include "stringpiece.h"
#include "ustringpiece.h"
#include "lv5/error_check.h"
#include "lv5/error.h"
#include "lv5/jsobject.h"
#include "lv5/jsarray.h"
#include "lv5/jsstring.h"
#include "lv5/map.h"
#include "lv5/jsregexp_impl.h"
#include "lv5/context_utils.h"
#include "lv5/match_result.h"
#include "lv5/bind.h"
namespace iv {
namespace lv5 {

class JSRegExp : public JSObject {
 public:
  JSRegExp(Context* ctx,
           const core::UStringPiece& value,
           const core::UStringPiece& flags)
    : JSObject(context::GetRegExpMap(ctx)),
      impl_(new JSRegExpImpl(value, flags)) {
    InitializeProperty(ctx, JSString::New(ctx, value));
  }

  JSRegExp(Context* ctx,
           const core::UStringPiece& value,
           const JSRegExpImpl* reg)
    : JSObject(context::GetRegExpMap(ctx)),
      impl_(reg) {
    InitializeProperty(ctx, JSString::New(ctx, value));
  }

  explicit JSRegExp(Context* ctx)
    : JSObject(context::GetRegExpMap(ctx)),
      impl_(new JSRegExpImpl()) {
    InitializeProperty(ctx, JSString::NewAsciiString(ctx, "(?:)"));
  }

  inline bool IsValid() const {
    return impl_->IsValid();
  }

  JSString* source(Context* ctx) {
    Error e;
    const JSVal source = Get(ctx, context::Intern(ctx, "source"), &e);
    assert(!e);
    assert(source.IsString());
    return source.string();
  }

  static JSRegExp* New(Context* ctx) {
    JSRegExp* const reg = new JSRegExp(ctx);
    reg->set_cls(JSRegExp::GetClass());
    reg->set_prototype(context::GetClassSlot(ctx, Class::RegExp).prototype);
    return reg;
  }

  static JSRegExp* New(Context* ctx,
                       const core::UStringPiece& value,
                       const JSRegExpImpl* impl) {
    JSRegExp* const reg = new JSRegExp(ctx, value, impl);
    reg->set_cls(JSRegExp::GetClass());
    reg->set_prototype(context::GetClassSlot(ctx, Class::RegExp).prototype);
    return reg;
  }

  static JSRegExp* New(Context* ctx,
                       const core::UStringPiece& value,
                       const core::UStringPiece& flags) {
    JSRegExp* const reg = new JSRegExp(ctx, value, flags);
    reg->set_cls(JSRegExp::GetClass());
    reg->set_prototype(context::GetClassSlot(ctx, Class::RegExp).prototype);
    return reg;
  }

  static JSRegExp* New(Context* ctx, JSRegExp* r) {
    const JSString* const source = r->source(ctx);
    JSRegExp* const reg = new JSRegExp(ctx, *source->GetFiber(), r->impl());
    reg->set_cls(JSRegExp::GetClass());
    reg->set_prototype(context::GetClassSlot(ctx, Class::RegExp).prototype);
    return reg;
  }

  static JSRegExp* NewPlain(Context* ctx) {
    return new JSRegExp(ctx);
  }

  bool global() const {
    return impl_->global();
  }

  bool ignore() const {
    return impl_->ignore();
  }

  bool multiline() const {
    return impl_->multiline();
  }

  int LastIndex(Context* ctx, Error* e) {
    const JSVal index = Get(ctx, context::Intern(ctx, "lastIndex"),
                            IV_LV5_ERROR_WITH(e, 0));
    const double val = index.ToNumber(ctx, IV_LV5_ERROR_WITH(e, 0));
    return core::DoubleToInt32(val);
  }

  void SetLastIndex(Context* ctx, int i, Error* e) {
    Put(ctx, context::Intern(ctx, "lastIndex"),
        static_cast<double>(i), true, IV_LV5_ERROR_VOID(e));
  }

  JSVal ExecuteOnce(Context* ctx,
                    const core::UStringPiece& piece,
                    int previous_index,
                    std::vector<int>* offset_vector,
                    Error* e) {
    const uint32_t num_of_captures = impl_->number_of_captures();
    const int rc = impl_->ExecuteOnce(piece, previous_index, offset_vector);
    if (rc == jscre::JSRegExpErrorNoMatch ||
        rc == jscre::JSRegExpErrorHitLimit) {
      return JSNull;
    }

    if (rc < 0) {
      e->Report(Error::Type, "RegExp execute failed");
      return JSUndefined;
    }

    JSArray* ary = JSArray::New(ctx, 2 * (num_of_captures + 1));
    for (int i = 0, len = 2 * (num_of_captures + 1); i < len; i += 2) {
      ary->DefineOwnProperty(
          ctx,
          symbol::MakeSymbolFromIndex(i),
          DataDescriptor((*offset_vector)[i],
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, IV_LV5_ERROR(e));
      ary->DefineOwnProperty(
          ctx,
          symbol::MakeSymbolFromIndex(i + 1),
          DataDescriptor((*offset_vector)[i+1],
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, IV_LV5_ERROR(e));
    }
    return ary;
  }

  JSVal ExecGlobal(Context* ctx, JSString* str, Error* e) {
    const uint32_t num_of_captures = impl_->number_of_captures();
    std::vector<int> offset_vector((num_of_captures + 1) * 3, -1);
    JSArray* ary = JSArray::New(ctx);
    SetLastIndex(ctx, 0, IV_LV5_ERROR(e));
    int previous_index = 0;
    int n = 0;
    const int start = previous_index;
    const int size = str->size();
    const JSString::Fiber* fiber = str->GetFiber();
    do {
      const int rc = impl_->ExecuteOnce(*fiber,
                                        previous_index, &offset_vector);
      if (rc == jscre::JSRegExpErrorNoMatch ||
          rc == jscre::JSRegExpErrorHitLimit) {
        break;
      }
      if (rc < 0) {
        e->Report(Error::Type, "RegExp execute failed");
        return JSUndefined;
      }
      const int this_index = offset_vector[1];
      if (previous_index == this_index) {
        ++previous_index;
      } else {
        previous_index = this_index;
      }
      if (previous_index > size || previous_index < 0) {
        break;
      }
      ary->DefineOwnProperty(
          ctx,
          symbol::MakeSymbolFromIndex(n),
          DataDescriptor(
              JSString::New(ctx,
                            fiber->begin() + offset_vector[0],
                            fiber->begin() + offset_vector[1]),
              PropertyDescriptor::WRITABLE |
              PropertyDescriptor::ENUMERABLE |
              PropertyDescriptor::CONFIGURABLE),
          true, IV_LV5_ERROR(e));
      ++n;
    } while (true);
    if (n == 0) {
      return JSNull;
    }
    ary->DefineOwnProperty(
        ctx,
        context::Intern(ctx, "index"),
        DataDescriptor(
            start,
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::ENUMERABLE |
            PropertyDescriptor::CONFIGURABLE),
        true, IV_LV5_ERROR(e));
    ary->DefineOwnProperty(
        ctx,
        context::Intern(ctx, "input"),
        DataDescriptor(
            str,
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::ENUMERABLE |
            PropertyDescriptor::CONFIGURABLE),
        true, IV_LV5_ERROR(e));
    return ary;
  }

  JSVal Exec(Context* ctx, JSString* str, Error* e) {
    const uint32_t num_of_captures = impl_->number_of_captures();
    std::vector<int> offset_vector((num_of_captures + 1) * 3, -1);
    const int start = LastIndex(ctx, IV_LV5_ERROR(e));  // for step 4
    int previous_index = start;
    if (!global()) {
      previous_index = 0;
    }
    const int size = str->size();
    if (previous_index > size || previous_index < 0) {
      SetLastIndex(ctx, 0, e);
      return JSNull;
    }
    const JSString::Fiber* fiber = str->GetFiber();
    const int rc = impl_->ExecuteOnce(*fiber,
                                      previous_index, &offset_vector);
    if (rc == jscre::JSRegExpErrorNoMatch ||
        rc == jscre::JSRegExpErrorHitLimit) {
      SetLastIndex(ctx, 0, e);
      return JSNull;
    }

    if (rc < 0) {
      e->Report(Error::Type, "RegExp execute failed");
      return JSUndefined;
    }

    previous_index = offset_vector[1];
    if (offset_vector[0] == offset_vector[1]) {
      ++previous_index;
    }

    if (global()) {
      SetLastIndex(ctx, previous_index, IV_LV5_ERROR(e));
    }

    JSArray* ary = JSArray::New(ctx, (num_of_captures + 1));
    ary->DefineOwnProperty(
        ctx,
        context::Intern(ctx, "index"),
        DataDescriptor(
            offset_vector[0],
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::ENUMERABLE |
            PropertyDescriptor::CONFIGURABLE),
        true, IV_LV5_ERROR(e));
    ary->DefineOwnProperty(
        ctx,
        context::Intern(ctx, "input"),
        DataDescriptor(
            str,
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::ENUMERABLE |
            PropertyDescriptor::CONFIGURABLE),
        true, IV_LV5_ERROR(e));
    for (int i = 0, len = num_of_captures + 1; i < len; ++i) {
      const int begin = offset_vector[i*2];
      const int end = offset_vector[i*2+1];
      if (begin != -1 && end != -1) {
        ary->DefineOwnProperty(
            ctx,
            symbol::MakeSymbolFromIndex(i),
            DataDescriptor(
                JSString::New(ctx,
                              fiber->begin() + offset_vector[i*2],
                              fiber->begin() + offset_vector[i*2+1]),
                PropertyDescriptor::WRITABLE |
                PropertyDescriptor::ENUMERABLE |
                PropertyDescriptor::CONFIGURABLE),
            true, IV_LV5_ERROR(e));
      } else {
        ary->DefineOwnProperty(
            ctx,
            symbol::MakeSymbolFromIndex(i),
            DataDescriptor(JSUndefined,
                PropertyDescriptor::WRITABLE |
                PropertyDescriptor::ENUMERABLE |
                PropertyDescriptor::CONFIGURABLE),
            true, IV_LV5_ERROR(e));
      }
    }
    return ary;
  }

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
      return std::make_tuple(0, 0, false);
    }
    if (rc < 0) {
      return std::make_tuple(0, 0, false);
    }
    for (int i = 1, len = num_of_captures + 1; i < len; ++i) {
      result->push_back(
          std::make_pair(offset_vector[i*2], offset_vector[i*2+1]));
    }
    return std::make_tuple(offset_vector[0], offset_vector[1], true);
  }

  static const Class* GetClass() {
    static const Class cls = {
      "RegExp",
      Class::RegExp
    };
    return &cls;
  }

 private:
  void InitializeProperty(Context* ctx, JSString* src) {
    bind::Object(ctx, this)
        .def(context::Intern(ctx, "source"),
             src->empty() ?
               JSString::NewAsciiString(ctx, "(?:)") : src,
             bind::NONE)
        .def(context::Intern(ctx, "global"),
             JSVal::Bool(impl_->global()),
             bind::NONE)
        .def(context::Intern(ctx, "ignoreCase"),
             JSVal::Bool(impl_->ignore()),
             bind::NONE)
        .def(context::Intern(ctx, "multiline"),
             JSVal::Bool(impl_->multiline()),
             bind::NONE)
        .def(context::Intern(ctx, "lastIndex"),
             0.0,
             bind::W);
  }

  const JSRegExpImpl* impl() const {
    return impl_;
  }

  const JSRegExpImpl* impl_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSREGEXP_H_
