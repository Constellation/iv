#ifndef _IV_LV5_RUNTIME_ARRAY_H_
#define _IV_LV5_RUNTIME_ARRAY_H_
#include "arguments.h"
#include "jsval.h"
#include "error.h"
#include "jsarray.h"
#include "jsstring.h"
#include "conversions.h"
#include "runtime_object.h"

namespace iv {
namespace lv5 {
namespace runtime {

// section 15.4.1.1 Array([item0 [, item1 [, ...]]])
// section 15.4.2.1 new Array([item0 [, item1 [, ...]]])
// section 15.4.2.2 new Array(len)
inline JSVal ArrayConstructor(const Arguments& args, Error* error) {
// TODO(Constellation) this is mock function
  return JSArray::New(args.ctx());
}

// section 15.4.4.2 Array.prototype.toString()
inline JSVal ArrayToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Array.prototype.toString", args, error);
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
  const JSVal join = obj->Get(args.ctx(),
                              args.ctx()->Intern("join"), ERROR(error));
  if (join.IsCallable()) {
    return join.object()->AsCallable()->Call(
        Arguments(args.ctx(), obj), ERROR(error));
  } else {
    return ObjectToString(Arguments(args.ctx(), obj), ERROR(error));
  }
}

// section 15.4.4.4 Array.prototype.concat([item1[, item2[, ...]]])
inline JSVal ArrayToConcat(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Array.prototype.concat", args, error);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
  JSArray* const ary = JSArray::New(ctx);

  uint32_t n = 0;
  const Class& cls = ctx->Cls("Array");
  std::tr1::array<char, 20> buf;

  if (cls.name == obj->cls()) {
    JSObject* const elm = obj;
    uint32_t k = 0;
    const JSVal length = elm->Get(
        ctx,
        ctx->length_symbol(), ERROR(error));
    assert(length.IsNumber());  // Array always number
    const uint32_t len = core::DoubleToUInt32(length.number());
    while (k < len) {
      const Symbol index = ctx->Intern(
          core::StringPiece(
              buf.data(),
              std::snprintf(buf.data(), buf.size(), "%lu",
                            static_cast<unsigned long>(k))));  // NOLINT
      if (elm->HasProperty(index)) {
        const JSVal subelm = elm->Get(ctx, index, ERROR(error));
        ary->DefineOwnProperty(
            ctx,
            ctx->Intern(
                core::StringPiece(
                    buf.data(),
                    std::snprintf(
                        buf.data(), buf.size(), "%lu",
                        static_cast<unsigned long>(n)))),  // NOLINT
            DataDescriptor(subelm,
                           PropertyDescriptor::WRITABLE |
                           PropertyDescriptor::ENUMERABLE |
                           PropertyDescriptor::CONFIGURABLE),
            false, ERROR(error));
      }
      ++n;
      ++k;
    }
  } else {
    ary->DefineOwnProperty(
        ctx,
        ctx->Intern(
            core::StringPiece(
                buf.data(),
                std::snprintf(
                    buf.data(), buf.size(), "%lu",
                    static_cast<unsigned long>(n)))),  // NOLINT
        DataDescriptor(obj,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, ERROR(error));
    ++n;
  }

  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    if (it->IsObject() && cls.name == it->object()->cls()) {
      JSObject* const elm = it->object();
      uint32_t k = 0;
      const JSVal length = elm->Get(
          ctx,
          ctx->length_symbol(), ERROR(error));
      assert(length.IsNumber());  // Array always number
      const uint32_t len = core::DoubleToUInt32(length.number());
      while (k < len) {
        const Symbol index = ctx->Intern(
            core::StringPiece(
                buf.data(),
                std::snprintf(buf.data(), buf.size(), "%lu",
                              static_cast<unsigned long>(k))));  // NOLINT
        if (elm->HasProperty(index)) {
          const JSVal subelm = elm->Get(ctx, index, ERROR(error));
          ary->DefineOwnProperty(
              ctx,
              ctx->Intern(
                  core::StringPiece(
                      buf.data(),
                      std::snprintf(
                          buf.data(), buf.size(), "%lu",
                          static_cast<unsigned long>(n)))),  // NOLINT
              DataDescriptor(subelm,
                             PropertyDescriptor::WRITABLE |
                             PropertyDescriptor::ENUMERABLE |
                             PropertyDescriptor::CONFIGURABLE),
              false, ERROR(error));
        }
        ++n;
        ++k;
      }
    } else {
      ary->DefineOwnProperty(
          ctx,
          ctx->Intern(
              core::StringPiece(
                  buf.data(),
                  std::snprintf(
                      buf.data(), buf.size(), "%lu",
                      static_cast<unsigned long>(n)))),  // NOLINT
          DataDescriptor(*it,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, ERROR(error));
      ++n;
    }
  }
  return ary;
}

// section 15.4.4.5 Array.prototype.join(separator)
inline JSVal ArrayToJoin(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Array.prototype.join", args, error);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(error));
  const double val = length.ToNumber(ctx, ERROR(error));
  const uint32_t len = core::DoubleToUInt32(val);
  JSString* separator;
  if (args.size() > 0 && !args[0].IsUndefined()) {
    separator = args[0].ToString(ctx, ERROR(error));
  } else {
    separator = JSString::NewAsciiString(ctx, ",");
  }
  if (len == 0) {
    return JSString::NewEmptyString(ctx);
  }
  JSStringBuilder builder(ctx);
  {
    const JSVal element0 = obj->Get(ctx, ctx->Intern("0"), ERROR(error));
    if (!element0.IsUndefined() && !element0.IsNull()) {
      const JSString* const str = element0.ToString(ctx, ERROR(error));
      builder.Append(*str);
    }
  }
  uint32_t k = 1;
  std::tr1::array<char, 20> buf;
  while (k < len) {
    builder.Append(*separator);
    const int num = std::snprintf(buf.data(), buf.size(), "%lu",
                                  static_cast<unsigned long>(k));  // NOLINT
    const JSVal element = obj->Get(
        ctx,
        ctx->Intern(core::StringPiece(buf.data(), num)),
        ERROR(error));
    if (!element.IsUndefined() && !element.IsNull()) {
      const JSString* const str = element.ToString(ctx, ERROR(error));
      builder.Append(*str);
    }
    k += 1;
  }
  return builder.Build();
}

// section 15.4.4.6 Array.prototype.pop()
inline JSVal ArrayToPop(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Array.prototype.pop", args, error);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(error));
  const double val = length.ToNumber(ctx, ERROR(error));
  const uint32_t len = core::DoubleToUInt32(val);
  if (len == 0) {
    obj->Put(ctx, ctx->length_symbol(), 0.0, true, ERROR(error));
    return JSUndefined;
  } else {
    std::tr1::array<char, 20> buf;
    const uint32_t index = len - 1;
    const Symbol indx = ctx->Intern(
        core::StringPiece(
            buf.data(),
            std::snprintf(
                buf.data(), buf.size(), "%lu",
                static_cast<unsigned long>(index))));  // NOLINT
    const JSVal element = obj->Get(ctx, indx, ERROR(error));
    obj->Delete(indx, true, ERROR(error));
    obj->Put(ctx, ctx->length_symbol(),
             index, true, ERROR(error));
    return element;
  }
}

// section 15.4.4.7 Array.prototype.push([item1[, item2[, ...]]])
inline JSVal ArrayToPush(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Array.prototype.push", args, error);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(error));
  const double val = length.ToNumber(ctx, ERROR(error));
  uint32_t n = core::DoubleToUInt32(val);
  std::tr1::array<char, 20> buf;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it, ++n) {
    obj->Put(
        ctx,
        ctx->Intern(
            core::StringPiece(
                buf.data(),
                std::snprintf(
                    buf.data(), buf.size(), "%lu",
                    static_cast<unsigned long>(n)))),  // NOLINT
        *it,
        true, ERROR(error));
  }
  obj->Put(
      ctx,
      ctx->length_symbol(),
      n,
      true, ERROR(error));
  return n;
}

// section 15.4.4.8 Array.prototype.reverse()
inline JSVal ArrayToReverse(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("Array.prototype.reverse", args, error);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(error));
  const double val = length.ToNumber(ctx, ERROR(error));
  const uint32_t len = core::DoubleToUInt32(val);
  const uint32_t middle = len >> 1;
  uint32_t lower = 0;
  std::tr1::array<char, 20> buf;
  while (lower != middle) {
    const uint32_t upper = len - lower - 1;
    const Symbol lower_symbol = ctx->Intern(
        core::StringPiece(
            buf.data(),
            std::snprintf(
                buf.data(), buf.size(), "%lu",
                static_cast<unsigned long>(lower))));  // NOLINT
    const Symbol upper_symbol = ctx->Intern(
        core::StringPiece(
            buf.data(),
            std::snprintf(
                buf.data(), buf.size(), "%lu",
                static_cast<unsigned long>(upper))));  // NOLINT
    const JSVal lower_value = obj->Get(
        ctx,
        lower_symbol,
        ERROR(error));
    const JSVal upper_value = obj->Get(
        ctx,
        upper_symbol,
        ERROR(error));
    const bool lower_exists = obj->HasProperty(lower_symbol);
    const bool upper_exists = obj->HasProperty(upper_symbol);
    if (lower_exists && upper_exists) {
      obj->Put(ctx, lower_symbol, upper_value, true, ERROR(error));
      obj->Put(ctx, upper_symbol, lower_value, true, ERROR(error));
    } else if (!lower_exists && upper_exists) {
      obj->Put(ctx, lower_symbol, upper_value, true, ERROR(error));
      obj->Delete(upper_symbol, true, ERROR(error));
    } else if (lower_exists && !upper_exists) {
      obj->Delete(lower_symbol, true, ERROR(error));
      obj->Put(ctx, upper_symbol, lower_value, true, ERROR(error));
    } else {
      // no action is required
    }
    ++lower;
  }
  return obj;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_ARRAY_H_
