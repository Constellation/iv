#include "lv5/lv5.h"
#include "lv5/jsregexp.h"
#include "lv5/jsarray.h"
#include "lv5/jsstring.h"
#include "lv5/context.h"
namespace iv {
namespace lv5 {

JSRegExp::JSRegExp(Context* ctx,
                   const core::UStringPiece& value,
                   const core::UStringPiece& flags)
  : impl_(new JSRegExpImpl(value, flags)) {
  DefineOwnProperty(ctx, ctx->Intern("source"),
                    DataDescriptor(JSString::New(ctx, value),
                                   PropertyDescriptor::NONE),
                                   false, ctx->error());
  InitializeProperty(ctx);
}

JSRegExp::JSRegExp(Context* ctx,
                   const core::UStringPiece& value,
                   const JSRegExpImpl* reg)
  : impl_(reg) {
  DefineOwnProperty(ctx, ctx->Intern("source"),
                    DataDescriptor(JSString::New(ctx, value),
                                   PropertyDescriptor::NONE),
                                   false, ctx->error());
  InitializeProperty(ctx);
}

JSRegExp::JSRegExp(Context* ctx)
  : impl_(new JSRegExpImpl()) {
  DefineOwnProperty(ctx, ctx->Intern("source"),
                    DataDescriptor(JSString::NewEmptyString(ctx),
                                   PropertyDescriptor::NONE),
                                   false, ctx->error());
  InitializeProperty(ctx);
}

void JSRegExp::InitializeProperty(Context* ctx) {
  DefineOwnProperty(ctx, ctx->Intern("global"),
                    DataDescriptor(JSVal::Bool(impl_->global()),
                                   PropertyDescriptor::NONE),
                                   false, ctx->error());

  DefineOwnProperty(ctx, ctx->Intern("ignoreCase"),
                    DataDescriptor(JSVal::Bool(impl_->ignore()),
                                   PropertyDescriptor::NONE),
                                   false, ctx->error());

  DefineOwnProperty(ctx, ctx->Intern("multiline"),
                    DataDescriptor(JSVal::Bool(impl_->multiline()),
                                   PropertyDescriptor::NONE),
                                   false, ctx->error());

  DefineOwnProperty(ctx, ctx->Intern("lastIndex"),
                    DataDescriptor(0.0,
                                   PropertyDescriptor::WRITABLE),
                                   false, ctx->error());
}

JSString* JSRegExp::source(Context* ctx) {
  Error e;
  const JSVal source = Get(ctx, ctx->Intern("source"), &e);
  assert(!e);
  assert(source.IsString());
  return source.string();
}

int JSRegExp::LastIndex(Context* ctx, Error* e) {
  const JSVal index = Get(ctx, ctx->Intern("lastIndex"), ERROR_WITH(e, 0));
  const double val = index.ToNumber(ctx, ERROR_WITH(e, 0));
  return core::DoubleToInt32(val);
}

void JSRegExp::SetLastIndex(Context* ctx, int i, Error* e) {
  Put(ctx, ctx->Intern("lastIndex"),
      static_cast<double>(i), true, ERROR_VOID(e));
}

JSVal JSRegExp::ExecuteOnce(Context* ctx,
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
    ary->DefineOwnPropertyWithIndex(
        ctx,
        i,
        DataDescriptor((*offset_vector)[i],
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, ERROR(e));
    ary->DefineOwnPropertyWithIndex(
        ctx,
        i + 1,
        DataDescriptor((*offset_vector)[i+1],
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, ERROR(e));
  }
  return ary;
}

JSVal JSRegExp::Exec(Context* ctx, JSString* str, Error* e) {
  const uint32_t num_of_captures = impl_->number_of_captures();
  std::vector<int> offset_vector((num_of_captures + 1) * 3, -1);
  const int start = LastIndex(ctx, ERROR(e));  // for step 4
  int previous_index = start;
  if (!global()) {
    previous_index = 0;
  }
  const int size = str->size();
  if (previous_index > size || previous_index < 0) {
    SetLastIndex(ctx, 0, e);
    return JSNull;
  }
  const int rc = impl_->ExecuteOnce(str->piece(),
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
    SetLastIndex(ctx, previous_index, ERROR(e));
  }

  JSArray* ary = JSArray::New(ctx, (num_of_captures + 1));
  ary->DefineOwnProperty(
      ctx,
      ctx->Intern("index"),
      DataDescriptor(
          offset_vector[0],
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::ENUMERABLE |
          PropertyDescriptor::CONFIGURABLE),
      true, ERROR(e));
  ary->DefineOwnProperty(
      ctx,
      ctx->Intern("input"),
      DataDescriptor(
          str,
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::ENUMERABLE |
          PropertyDescriptor::CONFIGURABLE),
      true, ERROR(e));
  for (int i = 0, len = num_of_captures + 1; i < len; ++i) {
    const int begin = offset_vector[i*2];
    const int end = offset_vector[i*2+1];
    if (begin != -1 && end != -1) {
      ary->DefineOwnPropertyWithIndex(
          ctx,
          i,
          DataDescriptor(
              JSString::New(ctx,
                            str->begin() + offset_vector[i*2],
                            str->begin() + offset_vector[i*2+1]),
              PropertyDescriptor::WRITABLE |
              PropertyDescriptor::ENUMERABLE |
              PropertyDescriptor::CONFIGURABLE),
          true, ERROR(e));
    } else {
      ary->DefineOwnPropertyWithIndex(
          ctx,
          i,
          DataDescriptor(JSUndefined,
              PropertyDescriptor::WRITABLE |
              PropertyDescriptor::ENUMERABLE |
              PropertyDescriptor::CONFIGURABLE),
          true, ERROR(e));
    }
  }
  return ary;
}

JSVal JSRegExp::ExecGlobal(Context* ctx,
                           JSString* str,
                           Error* e) {
  const uint32_t num_of_captures = impl_->number_of_captures();
  std::vector<int> offset_vector((num_of_captures + 1) * 3, -1);
  JSArray* ary = JSArray::New(ctx);
  SetLastIndex(ctx, 0, ERROR(e));
  int previous_index = 0;
  int n = 0;
  const int start = previous_index;
  const int size = str->size();
  do {
    const int rc = impl_->ExecuteOnce(str->piece(),
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
    ary->DefineOwnPropertyWithIndex(
        ctx,
        n,
        DataDescriptor(
            JSString::New(ctx,
                          str->begin() + offset_vector[0],
                          str->begin() + offset_vector[1]),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::ENUMERABLE |
            PropertyDescriptor::CONFIGURABLE),
        true, ERROR(e));
    ++n;
  } while (true);
  if (n == 0) {
    return JSNull;
  }
  ary->DefineOwnProperty(
      ctx,
      ctx->Intern("index"),
      DataDescriptor(
          start,
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::ENUMERABLE |
          PropertyDescriptor::CONFIGURABLE),
      true, ERROR(e));
  ary->DefineOwnProperty(
      ctx,
      ctx->Intern("input"),
      DataDescriptor(
          str,
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::ENUMERABLE |
          PropertyDescriptor::CONFIGURABLE),
      true, ERROR(e));
  return ary;
}

JSRegExp* JSRegExp::New(Context* ctx) {
  JSRegExp* const reg = new JSRegExp(ctx);
  const Class& cls = ctx->Cls("RegExp");
  reg->set_class_name(cls.name);
  reg->set_prototype(cls.prototype);
  return reg;
}

JSRegExp* JSRegExp::New(Context* ctx,
                        const core::UStringPiece& value,
                        const JSRegExpImpl* impl) {
  JSRegExp* const reg = new JSRegExp(ctx, value, impl);
  const Class& cls = ctx->Cls("RegExp");
  reg->set_class_name(cls.name);
  reg->set_prototype(cls.prototype);
  return reg;
}

JSRegExp* JSRegExp::New(Context* ctx,
                        const core::UStringPiece& value,
                        const core::UStringPiece& flags) {
  JSRegExp* const reg = new JSRegExp(ctx, value, flags);
  const Class& cls = ctx->Cls("RegExp");
  reg->set_class_name(cls.name);
  reg->set_prototype(cls.prototype);
  return reg;
}

JSRegExp* JSRegExp::New(Context* ctx, JSRegExp* r) {
  const JSString* const source = r->source(ctx);
  JSRegExp* const reg = new JSRegExp(ctx, source->piece(), r->impl());
  const Class& cls = ctx->Cls("RegExp");
  reg->set_class_name(cls.name);
  reg->set_prototype(cls.prototype);
  return reg;
}

JSRegExp* JSRegExp::NewPlain(Context* ctx) {
  return new JSRegExp(ctx);
}

} }  // namespace iv::lv5
