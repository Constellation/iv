#ifndef IV_LV5_JSREGEXP_H_
#define IV_LV5_JSREGEXP_H_
#include <vector>
#include <utility>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/map.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsregexp_impl.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/match_result.h>
#include <iv/lv5/bind.h>
namespace iv {
namespace lv5 {

class JSRegExp : public JSObject {
 public:
  inline bool IsValid() const {
    return impl_->IsValid();
  }

  JSString* source(Context* ctx) {
    Error e;
    const JSVal source = Get(ctx, symbol::source(), &e);
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

  static JSRegExp* New(Context* ctx, JSString* value) {
    JSRegExp* const reg = new JSRegExp(ctx, value);
    reg->set_cls(JSRegExp::GetClass());
    reg->set_prototype(context::GetClassSlot(ctx, Class::RegExp).prototype);
    return reg;
  }

  static JSRegExp* New(Context* ctx, JSString* value, JSString* flags) {
    JSRegExp* const reg = new JSRegExp(ctx, value, flags);
    reg->set_cls(JSRegExp::GetClass());
    reg->set_prototype(context::GetClassSlot(ctx, Class::RegExp).prototype);
    return reg;
  }

  static JSRegExp* New(Context* ctx, JSRegExp* r) {
    JSString* source = r->source(ctx);
    JSRegExp* const reg = new JSRegExp(ctx, source, r->impl());
    reg->set_cls(JSRegExp::GetClass());
    reg->set_prototype(context::GetClassSlot(ctx, Class::RegExp).prototype);
    return reg;
  }

  static JSRegExp* NewPlain(Context* ctx, Map* map) {
    return new JSRegExp(ctx, map);
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
    const JSVal index =
        Get(ctx, symbol::lastIndex(), IV_LV5_ERROR_WITH(e, 0));
    return index.ToInt32(ctx, e);
  }

  void SetLastIndex(Context* ctx, int i, Error* e) {
    Put(ctx, symbol::lastIndex(),
        static_cast<double>(i), true, IV_LV5_ERROR_VOID(e));
  }

  JSVal ExecGlobal(Context* ctx, JSString* str, Error* e) {
    if (str->Is8Bit()) {
      return ExecGlobal(ctx, str, str->Get8Bit(), e);
    } else {
      return ExecGlobal(ctx, str, str->Get16Bit(), e);
    }
  }

  JSVal Exec(Context* ctx, JSString* str, Error* e) {
    if (str->Is8Bit()) {
      return Exec(ctx, str, str->Get8Bit(), e);
    } else {
      return Exec(ctx, str, str->Get16Bit(), e);
    }
  }

  regexp::MatchResult Match(Context* ctx,
                            JSString* str,
                            int index,
                            regexp::PairVector* result) const {
    result->clear();
    const int num_of_captures = impl_->number_of_captures();
    std::vector<int> offset_vector(num_of_captures * 2);
    int res;
    if (str->Is8Bit()) {
      res = impl_->ExecuteOnce(ctx, *str->Get8Bit(),
                               index, offset_vector.data());
    } else {
      res = impl_->ExecuteOnce(ctx, *str->Get16Bit(),
                               index, offset_vector.data());
    }
    if (res == aero::AERO_FAILURE || res == aero::AERO_ERROR) {
      return std::make_tuple(0, 0, false);
    }
    for (int i = 1, len = num_of_captures; i < len; ++i) {
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
  JSRegExp(Context* ctx, JSString* pattern, JSString* flags)
    : JSObject(context::GetRegExpMap(ctx)),
      impl_() {
    int f = 0;
    if (flags->Is8Bit()) {
      const Fiber8* fiber8 = flags->Get8Bit();
      f = JSRegExpImpl::ComputeFlags(fiber8->begin(), fiber8->end());
    } else {
      const Fiber16* fiber16 = flags->Get16Bit();
      f = JSRegExpImpl::ComputeFlags(fiber16->begin(), fiber16->end());
    }
    impl_ = CompileImpl(ctx->regexp_allocator(), pattern, f);
    InitializeProperty(ctx, Escape(ctx, pattern));
  }

  JSRegExp(Context* ctx, JSString* pattern)
    : JSObject(context::GetRegExpMap(ctx)),
      impl_(CompileImpl(ctx->regexp_allocator(), pattern)) {
    InitializeProperty(ctx, Escape(ctx, pattern));
  }

  JSRegExp(Context* ctx,
           const core::UStringPiece& pattern,
           const JSRegExpImpl* reg)
    : JSObject(context::GetRegExpMap(ctx)),
      impl_(reg) {
    InitializeProperty(ctx, Escape(ctx, pattern));
  }

  JSRegExp(Context* ctx,
           JSString* source,
           const JSRegExpImpl* reg)
    : JSObject(context::GetRegExpMap(ctx)),
      impl_(reg) {
    InitializeProperty(ctx, source);
  }

  explicit JSRegExp(Context* ctx)
    : JSObject(context::GetRegExpMap(ctx)),
      impl_(new JSRegExpImpl(ctx->regexp_allocator())) {
    InitializeProperty(ctx, JSString::NewAsciiString(ctx, "(?:)"));
  }

  explicit JSRegExp(Context* ctx, Map* map)
    : JSObject(map),
      impl_(new JSRegExpImpl(ctx->regexp_allocator())) {
    InitializeProperty(ctx, JSString::NewAsciiString(ctx, "(?:)"));
  }

  static JSString* Escape(Context* ctx, JSString* str) {
    JSStringBuilder builder;
    builder.reserve(str->size());
    bool is_8bit = true;
    if (str->Is8Bit()) {
      const Fiber8* fiber8 = str->Get8Bit();
      core::RegExpEscape(fiber8->begin(), fiber8->end(),
                         std::back_inserter(builder));
    } else {
      const Fiber16* fiber16 = str->Get16Bit();
      core::RegExpEscape(fiber16->begin(), fiber16->end(),
                         std::back_inserter(builder));
      is_8bit = false;
    }
    return builder.Build(ctx, is_8bit);
  }

  static JSString* Escape(Context* ctx, const core::UStringPiece& str) {
    JSStringBuilder builder;
    builder.reserve(str.size());
    core::RegExpEscape(str.begin(), str.end(),
                       std::back_inserter(builder));
    return builder.Build(ctx);
  }

  void InitializeProperty(Context* ctx, JSString* src) {
    bind::Object(ctx, this)
        .def(symbol::source(),
             src->empty() ? JSString::NewAsciiString(ctx, "(?:)") : src,
             ATTR::NONE)
        .def(symbol::global(),
             JSVal::Bool(impl_->global()), ATTR::NONE)
        .def(symbol::ignoreCase(),
             JSVal::Bool(impl_->ignore()), ATTR::NONE)
        .def(symbol::multiline(),
             JSVal::Bool(impl_->multiline()), ATTR::NONE)
        .def(symbol::lastIndex(), 0.0, ATTR::W);
  }

  template<typename FiberType>
  JSVal ExecGlobal(Context* ctx, JSString* str, const FiberType* fiber, Error* e) {
    const int num_of_captures = impl_->number_of_captures();
    std::vector<int> offset_vector((num_of_captures) * 2);
    JSArray* ary = JSArray::New(ctx);
    SetLastIndex(ctx, 0, IV_LV5_ERROR(e));
    int previous_index = 0;
    int n = 0;
    const int start = previous_index;
    const int size = fiber->size();
    do {
      const int res = impl_->ExecuteOnce(ctx,
                                         *fiber,
                                         previous_index,
                                         offset_vector.data());
      if (res == aero::AERO_FAILURE || res == aero::AERO_ERROR) {
        break;
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
              ATTR::W | ATTR::E | ATTR::C),
          true, IV_LV5_ERROR(e));
      ++n;
    } while (true);
    if (n == 0) {
      return JSNull;
    }
    ary->DefineOwnProperty(
        ctx,
        symbol::index(),
        DataDescriptor(start, ATTR::W | ATTR::E | ATTR::C),
        true, IV_LV5_ERROR(e));
    ary->DefineOwnProperty(
        ctx,
        symbol::input(),
        DataDescriptor(str, ATTR::W | ATTR::E | ATTR::C),
        true, IV_LV5_ERROR(e));
    return ary;
  }

  template<typename FiberType>
  JSVal Exec(Context* ctx, JSString* str, const FiberType* fiber, Error* e) {
    const int num_of_captures = impl_->number_of_captures();
    std::vector<int> offset_vector(num_of_captures * 2);
    const int start = LastIndex(ctx, IV_LV5_ERROR(e));  // for step 4
    int previous_index = start;
    if (!global()) {
      previous_index = 0;
    }
    const int size = fiber->size();
    if (previous_index > size || previous_index < 0) {
      SetLastIndex(ctx, 0, e);
      return JSNull;
    }
    const int res = impl_->ExecuteOnce(ctx,
                                       *fiber,
                                       previous_index,
                                       offset_vector.data());
    if (res == aero::AERO_FAILURE || res == aero::AERO_ERROR) {
      SetLastIndex(ctx, 0, e);
      return JSNull;
    }

    previous_index = offset_vector[1];
    if (offset_vector[0] == offset_vector[1]) {
      ++previous_index;
    }

    if (global()) {
      SetLastIndex(ctx, previous_index, IV_LV5_ERROR(e));
    }

    JSArray* ary = JSArray::New(ctx, num_of_captures);
    ary->DefineOwnProperty(
        ctx,
        symbol::index(),
        DataDescriptor(offset_vector[0], ATTR::W | ATTR::E | ATTR::C),
        true, IV_LV5_ERROR(e));
    ary->DefineOwnProperty(
        ctx,
        symbol::input(),
        DataDescriptor(str, ATTR::W | ATTR::E | ATTR::C),
        true, IV_LV5_ERROR(e));
    for (int i = 0; i < num_of_captures; ++i) {
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
                ATTR::W | ATTR::E | ATTR::C),
            true, IV_LV5_ERROR(e));
      } else {
        ary->DefineOwnProperty(
            ctx,
            symbol::MakeSymbolFromIndex(i),
            DataDescriptor(JSUndefined, ATTR::W | ATTR::E | ATTR::C),
            true, IV_LV5_ERROR(e));
      }
    }
    return ary;
  }

  static JSRegExpImpl* CompileImpl(core::Space* space,
                                   JSString* pattern, int flags = 0) {
    if (pattern->Is8Bit()) {
      return new JSRegExpImpl(space, *pattern->Get8Bit(), flags);
    } else {
      return new JSRegExpImpl(space, *pattern->Get16Bit(), flags);
    }
  }

  const JSRegExpImpl* impl() const {
    return impl_;
  }

  const JSRegExpImpl* impl_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSREGEXP_H_
