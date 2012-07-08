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
#include <iv/lv5/bind.h>
#include <iv/lv5/jsvector.h>
namespace iv {
namespace lv5 {

class JSRegExp : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(RegExp)

  // see also global_data.h
  enum FIELD {
    FIELD_SOURCE = 0,
    FIELD_GLOBAL = 1,
    FIELD_IGNORE_CASE = 2,
    FIELD_MULTILINE = 3,
    FIELD_LAST_INDEX = 4
  };

  JSString* source(Context* ctx) {
    Error::Dummy e;
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

  bool IsValid() const { return impl_->IsValid(); }

  bool global() const { return impl_->global(); }

  bool ignore() const { return impl_->ignore(); }

  bool multiline() const { return impl_->multiline(); }

  bool sticky() const { return impl_->sticky(); }

  int LastIndex(Context* ctx, Error* e) {
    // because not configurable
    assert(GetSlot(FIELD_LAST_INDEX).IsDataDescriptor());
    return
        GetSlot(FIELD_LAST_INDEX).AsDataDescriptor()->value().ToInt32(ctx, e);
  }

  void SetLastIndex(Context* ctx, int i, Error* e) {
    PutToSlotOffset(ctx, FIELD_LAST_INDEX, JSVal::Int32(i), true, e);
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

  bool Match(Context* ctx,
             JSString* str,
             int index,
             std::vector<int>* vec) const {
    assert(
        static_cast<std::size_t>(impl_->number_of_captures() * 2)
        <= vec->size());
    const int res = (str->Is8Bit()) ?
        impl_->Execute(ctx, *str->Get8Bit(), index, vec->data()) :
        impl_->Execute(ctx, *str->Get16Bit(), index, vec->data());
    return res == aero::AERO_SUCCESS;
  }

  uint32_t num_of_captures() const { return impl_->number_of_captures(); }

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
    // TODO(Constellation) cleanup logic
    bind::Object(ctx, this)
        .def(symbol::source(),
             JSString::NewAsciiString(ctx, "(?:)"),
             ATTR::NONE)
        .def(symbol::global(),
             JSVal::Bool(impl_->global()), ATTR::NONE)
        .def(symbol::ignoreCase(),
             JSVal::Bool(impl_->ignore()), ATTR::NONE)
        .def(symbol::multiline(),
             JSVal::Bool(impl_->multiline()), ATTR::NONE)
        .def(symbol::lastIndex(), 0.0, ATTR::W);
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
    GetSlot(FIELD_SOURCE) =
        DataDescriptor(
            src->empty() ? JSString::NewAsciiString(ctx, "(?:)") : src,
            ATTR::NONE);
    GetSlot(FIELD_GLOBAL) =
        DataDescriptor(JSVal::Bool(impl_->global()), ATTR::NONE);
    GetSlot(FIELD_IGNORE_CASE) =
        DataDescriptor(JSVal::Bool(impl_->ignore()), ATTR::NONE);
    GetSlot(FIELD_MULTILINE) =
        DataDescriptor(JSVal::Bool(impl_->multiline()), ATTR::NONE);
    GetSlot(FIELD_LAST_INDEX) =
        DataDescriptor(JSVal::Int32(0), ATTR::W);
  }

  template<typename FiberType>
  JSVal ExecGlobal(Context* ctx, JSString* str,
                   const FiberType* fiber, Error* e) {
    const int num_of_captures = impl_->number_of_captures();
    std::vector<int> offset_vector((num_of_captures) * 2);
    JSVector* vec = JSVector::New(ctx);
    vec->reserve(32);
    SetLastIndex(ctx, 0, IV_LV5_ERROR(e));
    int previous_index = 0;
    const int size = fiber->size();
    do {
      const int res =
          impl_->Execute(ctx, *fiber, previous_index, offset_vector.data());
      if (res != aero::AERO_SUCCESS) {
        break;
      }

      int last_index = offset_vector[1];
      if (offset_vector[0] == offset_vector[1]) {
        ++last_index;
      }

      const int this_index = last_index;
      if (previous_index == this_index) {
        ++previous_index;
      } else {
        previous_index = this_index;
      }
      if (previous_index > size) {
        break;
      }
      vec->push_back(JSString::NewWithFiber(ctx, fiber, offset_vector[0], offset_vector[1]));
    } while (true);

    if (vec->empty()) {
      return JSNull;
    }

    // set index and input
    JSArray* ary = vec->ToJSArray();
    ary->JSArray::DefineOwnProperty(
        ctx,
        symbol::index(),
        DataDescriptor(JSVal::Int32(0), ATTR::W | ATTR::E | ATTR::C),
        true, IV_LV5_ERROR(e));
    ary->JSArray::DefineOwnProperty(
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

    const int res =
        impl_->Execute(ctx, *fiber, previous_index, offset_vector.data());

    if (res != aero::AERO_SUCCESS) {
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

    JSVector* vec = JSVector::New(ctx, num_of_captures);
    for (int i = 0; i < num_of_captures; ++i) {
      const int begin = offset_vector[i*2];
      const int end = offset_vector[i*2+1];
      if (begin != -1 && end != -1) {
        (*vec)[i] = JSString::NewWithFiber(ctx, fiber, begin, end);
      }
    }

    // set index and input
    JSArray* ary = vec->ToJSArray();
    ary->JSArray::DefineOwnProperty(
        ctx,
        symbol::index(),
        DataDescriptor(offset_vector[0], ATTR::W | ATTR::E | ATTR::C),
        true, IV_LV5_ERROR(e));
    ary->JSArray::DefineOwnProperty(
        ctx,
        symbol::input(),
        DataDescriptor(str, ATTR::W | ATTR::E | ATTR::C),
        true, IV_LV5_ERROR(e));
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
