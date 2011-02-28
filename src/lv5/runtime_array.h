#ifndef _IV_LV5_RUNTIME_ARRAY_H_
#define _IV_LV5_RUNTIME_ARRAY_H_
#include "conversions.h"
#include "lv5/lv5.h"
#include "lv5/arguments.h"
#include "lv5/internal.h"
#include "lv5/jsval.h"
#include "lv5/error.h"
#include "lv5/jsarray.h"
#include "lv5/jsstring.h"
#include "lv5/internal.h"
#include "lv5/runtime_object.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

inline JSVal CompareFn(const Arguments& args, Error* e) {
  assert(args.size() == 2);  // always 2
  const JSVal& lhs = args[0];
  const JSVal& rhs = args[1];
  if (StrictEqual(lhs, rhs)) {
    return 0.0;
  } else if (lhs.IsNumber() && rhs.IsNumber()) {
    if (lhs.number() == rhs.number()) {
      return 0.0;
    }
    return lhs.number() < rhs.number() ? -1.0 : 1.0;
  }
  const JSString* const lhs_str = lhs.ToString(args.ctx(), ERROR(e));
  const JSString* const rhs_str = rhs.ToString(args.ctx(), ERROR(e));
  if (*lhs_str == *rhs_str) {
    return 0.0;
  }
  return (*lhs_str < *rhs_str) ? -1.0 : 1.0;
}

}  // namespace iv::lv5::runtime::detail

// section 15.4.1.1 Array([item0 [, item1 [, ...]]])
// section 15.4.2.1 new Array([item0 [, item1 [, ...]]])
// section 15.4.2.2 new Array(len)
inline JSVal ArrayConstructor(const Arguments& args, Error* e) {
  const std::size_t args_size = args.size();
  Context* ctx = args.ctx();
  if (args_size == 0) {
    return JSArray::New(ctx);
  }
  if (args_size == 1) {
    const JSVal& first = args[0];
    if (first.IsNumber()) {
      const double val = first.ToNumber(ctx, ERROR(e));
      const uint32_t len = core::DoubleToUInt32(val);
      if (val == len) {
        return JSArray::New(ctx, len);
      } else {
        e->Report(Error::Range,
                  "invalid array length");
        return JSUndefined;
      }
    } else {
      JSArray* const ary = JSArray::New(ctx, 1);
      ary->DefineOwnPropertyWithIndex(
          ctx, 0,
          DataDescriptor(first, PropertyDescriptor::WRITABLE |
                                PropertyDescriptor::ENUMERABLE |
                                PropertyDescriptor::CONFIGURABLE),
          false, ERROR(e));
      return ary;
    }
  } else {
    JSArray* const ary =JSArray::New(ctx, args.size());
    uint32_t index = 0;
    for (Arguments::const_iterator it = args.begin(),
         last = args.end(); it != last; ++it, ++index) {
      ary->DefineOwnPropertyWithIndex(
          ctx, index,
          DataDescriptor(*it, PropertyDescriptor::WRITABLE |
                              PropertyDescriptor::ENUMERABLE |
                              PropertyDescriptor::CONFIGURABLE),
          false, ERROR(e));
    }
    return ary;
  }
}

// section 15.4.3.2 Array.isArray(arg)
inline JSVal ArrayIsArray(const Arguments& args, Error* e) {
  if (args.size() == 0) {
    return JSFalse;
  }
  const JSVal& val = args[0];
  if (!val.IsObject()) {
    return JSFalse;
  }
  return JSVal::Bool(args.ctx()->IsArray(*val.object()));
}

// section 15.4.4.2 Array.prototype.toString()
inline JSVal ArrayToString(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.toString", args, e);
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(e));
  const JSVal join = obj->Get(args.ctx(),
                              args.ctx()->Intern("join"), ERROR(e));
  if (join.IsCallable()) {
    Arguments a(args.ctx(), ERROR(e));
    return join.object()->AsCallable()->Call(a, obj, e);
  } else {
    Arguments a(args.ctx(), ERROR(e));
    a.set_this_binding(obj);
    return ObjectToString(a, e);
  }
}

// section 15.4.4.3 Array.prototype.toLocaleString()
inline JSVal ArrayToLocaleString(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.toLocaleString", args, e);
  Context* const ctx = args.ctx();
  JSObject* const array = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = array->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t len = core::DoubleToUInt32(val);
  if (len == 0) {
    return JSString::NewEmptyString(ctx);
  }

  // implementation depended locale based separator
  const char separator = ',';
  const Symbol toLocaleString = ctx->Intern("toLocaleString");
  Arguments args_list(ctx, ERROR(e));
  StringBuilder builder;
  {
    const JSVal first = array->GetWithIndex(ctx, 0, ERROR(e));
    if (!first.IsUndefined() && !first.IsNull()) {
      JSObject* const elm_obj = first.ToObject(ctx, ERROR(e));
      const JSVal method = elm_obj->Get(ctx, toLocaleString, ERROR(e));
      if (!method.IsCallable()) {
        e->Report(Error::Type, "toLocaleString is not function");
        return JSUndefined;
      }
      const JSVal R = method.object()->AsCallable()->Call(args_list,
                                                          elm_obj,
                                                          ERROR(e));
      const JSString* const str = R.ToString(ctx, ERROR(e));
      builder.Append(*str);
    }
  }

  uint32_t k = 1;
  while (k < len) {
    builder.Append(separator);
    const JSVal element = array->GetWithIndex(
        ctx,
        k,
        ERROR(e));
    if (!element.IsUndefined() && !element.IsNull()) {
      JSObject* const elm_obj = element.ToObject(ctx, ERROR(e));
      const JSVal method = elm_obj->Get(ctx, toLocaleString, ERROR(e));
      if (!method.IsCallable()) {
        e->Report(Error::Type, "toLocaleString is not function");
        return JSUndefined;
      }
      const JSVal R = method.object()->AsCallable()->Call(args_list,
                                                          elm_obj,
                                                          ERROR(e));
      const JSString* const str = R.ToString(ctx, ERROR(e));
      builder.Append(*str);
    }
    ++k;
  }
  return builder.Build(ctx);
}

// section 15.4.4.4 Array.prototype.concat([item1[, item2[, ...]]])
inline JSVal ArrayConcat(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.concat", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  JSArray* const ary = JSArray::New(ctx);

  uint32_t n = 0;
  if (ctx->IsArray(*obj)) {
    JSObject* const elm = obj;
    uint32_t k = 0;
    const JSVal length = elm->Get(
        ctx,
        ctx->length_symbol(), ERROR(e));
    assert(length.IsNumber());  // Array always number
    const uint32_t len = core::DoubleToUInt32(length.number());
    while (k < len) {
      if (elm->HasPropertyWithIndex(ctx, k)) {
        const JSVal subelm = elm->GetWithIndex(ctx, k, ERROR(e));
        ary->DefineOwnPropertyWithIndex(
            ctx,
            n,
            DataDescriptor(subelm,
                           PropertyDescriptor::WRITABLE |
                           PropertyDescriptor::ENUMERABLE |
                           PropertyDescriptor::CONFIGURABLE),
            false, ERROR(e));
      }
      ++n;
      ++k;
    }
  } else {
    ary->DefineOwnPropertyWithIndex(
        ctx,
        n,
        DataDescriptor(obj,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, ERROR(e));
    ++n;
  }

  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    if (it->IsObject() && ctx->IsArray(*it->object())) {
      JSObject* const elm = it->object();
      uint32_t k = 0;
      const JSVal length = elm->Get(
          ctx,
          ctx->length_symbol(), ERROR(e));
      assert(length.IsNumber());  // Array always number
      const uint32_t len = core::DoubleToUInt32(length.number());
      while (k < len) {
        if (elm->HasPropertyWithIndex(ctx, k)) {
          const JSVal subelm = elm->GetWithIndex(ctx, k, ERROR(e));
          ary->DefineOwnPropertyWithIndex(
              ctx,
              n,
              DataDescriptor(subelm,
                             PropertyDescriptor::WRITABLE |
                             PropertyDescriptor::ENUMERABLE |
                             PropertyDescriptor::CONFIGURABLE),
              false, ERROR(e));
        }
        ++n;
        ++k;
      }
    } else {
      ary->DefineOwnPropertyWithIndex(
          ctx,
          n,
          DataDescriptor(*it,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, ERROR(e));
      ++n;
    }
  }
  ary->Put(ctx, ctx->length_symbol(),
           n, false, ERROR(e));
  return ary;
}

// section 15.4.4.5 Array.prototype.join(separator)
inline JSVal ArrayJoin(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.join", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t len = core::DoubleToUInt32(val);
  JSString* separator;
  if (args.size() > 0 && !args[0].IsUndefined()) {
    separator = args[0].ToString(ctx, ERROR(e));
  } else {
    separator = JSString::NewAsciiString(ctx, ",");
  }
  if (len == 0) {
    return JSString::NewEmptyString(ctx);
  }
  StringBuilder builder;
  {
    const JSVal element0 = obj->GetWithIndex(ctx, 0, ERROR(e));
    if (!element0.IsUndefined() && !element0.IsNull()) {
      const JSString* const str = element0.ToString(ctx, ERROR(e));
      builder.Append(*str);
    }
  }
  uint32_t k = 1;
  while (k < len) {
    builder.Append(*separator);
    const JSVal element = obj->GetWithIndex(
        ctx,
        k,
        ERROR(e));
    if (!element.IsUndefined() && !element.IsNull()) {
      const JSString* const str = element.ToString(ctx, ERROR(e));
      builder.Append(*str);
    }
    ++k;
  }
  return builder.Build(ctx);
}

// section 15.4.4.6 Array.prototype.pop()
inline JSVal ArrayPop(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.pop", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t len = core::DoubleToUInt32(val);
  if (len == 0) {
    obj->Put(ctx, ctx->length_symbol(), 0.0, true, ERROR(e));
    return JSUndefined;
  } else {
    const uint32_t index = len - 1;
    const JSVal element = obj->GetWithIndex(ctx, index, ERROR(e));
    obj->DeleteWithIndex(ctx, index, true, ERROR(e));
    obj->Put(ctx, ctx->length_symbol(),
             index, true, ERROR(e));
    return element;
  }
}

// section 15.4.4.7 Array.prototype.push([item1[, item2[, ...]]])
inline JSVal ArrayPush(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.push", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  uint32_t n = core::DoubleToUInt32(val);
  bool index_over = false;
  Arguments::const_iterator it = args.begin();
  const Arguments::const_iterator last = args.end();
  for (; it != last; ++it, ++n) {
    obj->PutWithIndex(
        ctx,
        n,
        *it,
        true, ERROR(e));
    if (n == UINT32_MAX) {
      index_over = true;
      break;
    }
  }
  double len = static_cast<double>(n);
  if (index_over) {
    for (++len, ++it; it != last; ++it, ++len) {
      obj->Put(
          ctx,
          ctx->InternDouble(len),
          *it,
          true, ERROR(e));
    }
  }
  obj->Put(
      ctx,
      ctx->length_symbol(),
      len,
      true, ERROR(e));
  return len;
}

// section 15.4.4.8 Array.prototype.reverse()
inline JSVal ArrayReverse(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.reverse", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t len = core::DoubleToUInt32(val);
  const uint32_t middle = len >> 1;
  for (uint32_t lower = 0; lower != middle; ++lower) {
    const uint32_t upper = len - lower - 1;
    const JSVal lower_value = obj->GetWithIndex(
        ctx,
        lower,
        ERROR(e));
    const JSVal upper_value = obj->GetWithIndex(
        ctx,
        upper,
        ERROR(e));
    const bool lower_exists = obj->HasPropertyWithIndex(ctx, lower);
    const bool upper_exists = obj->HasPropertyWithIndex(ctx, upper);
    if (lower_exists && upper_exists) {
      obj->PutWithIndex(ctx, lower, upper_value, true, ERROR(e));
      obj->PutWithIndex(ctx, upper, lower_value, true, ERROR(e));
    } else if (!lower_exists && upper_exists) {
      obj->PutWithIndex(ctx, lower, upper_value, true, ERROR(e));
      obj->DeleteWithIndex(ctx, upper, true, ERROR(e));
    } else if (lower_exists && !upper_exists) {
      obj->DeleteWithIndex(ctx, lower, true, ERROR(e));
      obj->PutWithIndex(ctx, upper, lower_value, true, ERROR(e));
    } else {
      // no action is required
    }
  }
  return obj;
}

// section 15.4.4.9 Array.prototype.shift()
inline JSVal ArrayShift(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.shift", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t len = core::DoubleToUInt32(val);
  if (len == 0) {
    obj->Put(ctx, ctx->length_symbol(), 0.0, true, ERROR(e));
    return JSUndefined;
  }
  const JSVal first = obj->Get(ctx, ctx->Intern("0"), ERROR(e));
  uint32_t to = 0;
  uint32_t from = 0;
  for (uint32_t k = 1; k < len; ++k, to = from) {
    from = k;
    if (obj->HasPropertyWithIndex(ctx, from)) {
      const JSVal from_value = obj->GetWithIndex(ctx, from, ERROR(e));
      obj->PutWithIndex(ctx, to, from_value, true, ERROR(e));
    } else {
      obj->DeleteWithIndex(ctx, to, true, ERROR(e));
    }
  }
  obj->DeleteWithIndex(ctx, from, true, ERROR(e));
  obj->Put(ctx, ctx->length_symbol(), len - 1, true, ERROR(e));
  return first;
}

// section 15.4.4.10 Array.prototype.slice(start, end)
inline JSVal ArraySlice(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.slice", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  JSArray* const ary = JSArray::New(ctx);
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t len = core::DoubleToUInt32(val);
  uint32_t k;
  if (args.size() > 0) {
    double relative_start = args[0].ToNumber(ctx, ERROR(e));
    relative_start = core::DoubleToInteger(relative_start);
    if (relative_start < 0) {
      k = core::DoubleToUInt32(std::max<double>(relative_start + len, 0.0));
    } else {
      k = core::DoubleToUInt32(std::min<double>(relative_start, len));
    }
  } else {
    k = 0;
  }
  uint32_t final;
  if (args.size() > 1) {
    if (args[1].IsUndefined()) {
      final = len;
    } else {
      double relative_end = args[1].ToNumber(ctx, ERROR(e));
      relative_end = core::DoubleToInteger(relative_end);
      if (relative_end < 0) {
        final = core::DoubleToUInt32(std::max<double>(relative_end + len, 0.0));
      } else {
        final = core::DoubleToUInt32(std::min<double>(relative_end, len));
      }
    }
  } else {
    final = len;
  }
  for (uint32_t n = 0; k < final; ++k, ++n) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      const JSVal kval = obj->GetWithIndex(ctx, k, ERROR(e));
      ary->DefineOwnPropertyWithIndex(
          ctx,
          n,
          DataDescriptor(kval,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, ERROR(e));
    }
  }
  return ary;
}

// section 15.4.4.11 Array.prototype.sort(comparefn)
// non recursive quick sort
inline JSVal ArraySort(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.sort", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t len = core::DoubleToUInt32(val);
  JSFunction* comparefn;
  if (args.size() > 0 && args[0].IsCallable()) {
    comparefn = args[0].object()->AsCallable();
  } else {
    comparefn = JSInlinedFunction<&detail::CompareFn, 2>::New(ctx);
  }
  if (len == 0) {
    return obj;
  }
  {
    // non recursive quick sort
    int sp = 1;
    int64_t l = 0, r = 0;
    const JSVal zero(0.0);
    static const int32_t kStackSize = 32;
    std::tr1::array<int64_t, kStackSize> lstack, rstack;
    lstack[0] = 0;
    rstack[0] = len - 1;
    Arguments a(ctx, 2, ERROR(e));
    while (sp > 0) {
      --sp;
      l = lstack[sp];
      r = rstack[sp];

      if (l < r) {
        if (r - l < 20) {
          // only 20 elements. using insertion sort
          for (int64_t i = l + 1; i <= r; ++i) {
            const JSVal t = obj->GetWithIndex(ctx, static_cast<uint32_t>(i), ERROR(e));
            int64_t j = i - 1;
            for (; j >= l; --j) {
              const JSVal t2 = obj->GetWithIndex(ctx, static_cast<uint32_t>(j), ERROR(e));
              a[0] = t2;
              a[1] = t;
              const JSVal res = comparefn->Call(a, JSUndefined, ERROR(e));
              const CompareKind kind =
                  Compare<false>(ctx, zero, res, ERROR(e));  // res > zero
              if (kind == CMP_TRUE) {  // res > zero is true
                obj->PutWithIndex(ctx,
                                  static_cast<uint32_t>(j + 1), t2, true, ERROR(e));
              } else {
                break;
              }
            }
            obj->PutWithIndex(ctx, static_cast<uint32_t>(j + 1), t, true, ERROR(e));
          }
        } else {
          // quick sort
          const int64_t pivot = (l + r) >> 1;
          const JSVal s = obj->GetWithIndex(ctx, pivot, ERROR(e));
          a[1] = s;
          int64_t i = l - 1;
          int64_t j = r + 1;
          while (true) {
            // search from left
            while (true) {
              ++i;
              // if compare func has very storange behavior,
              // prevent by length
              if (i == r) {
                break;
              }
              const JSVal target =
                  obj->GetWithIndex(ctx, static_cast<uint32_t>(i), ERROR(e));
              a[0] = target;
              // if res < 0, next
              const JSVal res = comparefn->Call(a, JSUndefined, ERROR(e));
              const CompareKind kind =
                  Compare<true>(ctx, res, zero, ERROR(e));
              if (kind != CMP_TRUE) {
                break;
              }
            }
            // search from right
            while (true) {
              --j;
              // if compare func has very storange behavior,
              // prevent by length
              if (j == l) {
                break;
              }
              const JSVal target =
                  obj->GetWithIndex(ctx, static_cast<uint32_t>(j), ERROR(e));
              a[0] = target;
              // if res > 0, next
              const JSVal res = comparefn->Call(a, JSUndefined, ERROR(e));
              const CompareKind kind =
                  Compare<false>(ctx, zero, res, ERROR(e));  // res > zero
              if (kind != CMP_TRUE) {  // target < s is true
                break;
              }
            }

            if (i >= j) {
              break;
            }

            // swap
            const JSVal ival =
                obj->GetWithIndex(ctx, static_cast<uint32_t>(i), ERROR(e));
            const JSVal jval =
                obj->GetWithIndex(ctx, static_cast<uint32_t>(j), ERROR(e));
            obj->PutWithIndex(ctx,
                              static_cast<uint32_t>(i), jval, true, ERROR(e));
            obj->PutWithIndex(ctx,
                              static_cast<uint32_t>(j), ival, true, ERROR(e));
          }

          if (sp + 2 > kStackSize) {
            e->Report(
                Error::Type,
                "Array.protoype.sort stack overflow");
            return JSUndefined;
          }

          if (i - l < r - i) {
            lstack[sp] = i;
            rstack[sp++] = r;
            lstack[sp] = l;
            rstack[sp++] = i - 1;
          } else {
            lstack[sp] = l;
            rstack[sp++] = i - 1;
            lstack[sp] = i;
            rstack[sp++] = r;
          }
        }
      }
    }
  }
  return obj;
}

// section 15.4.4.12
// Array.prototype.splice(start, deleteCount[, item1[, item2[, ...]]])
inline JSVal ArraySplice(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.splice", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  JSArray* const ary = JSArray::New(ctx);
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t len = core::DoubleToUInt32(val);
  const uint32_t args_size = args.size();
  uint32_t actual_start;
  if (args_size > 0) {
    double relative_start = args[0].ToNumber(ctx, ERROR(e));
    relative_start = core::DoubleToInteger(relative_start);
    if (relative_start < 0) {
      actual_start = core::DoubleToUInt32(
          std::max<double>(relative_start + len, 0.0));
    } else {
      actual_start = core::DoubleToUInt32(
          std::min<double>(relative_start, len));
    }
  } else {
    actual_start = 0;
  }
  uint32_t actual_delete_count;
  if (args_size > 1) {
    double delete_count = args[1].ToNumber(ctx, ERROR(e));
    delete_count = core::DoubleToInteger(delete_count);
    actual_delete_count = core::DoubleToUInt32(
        std::min<double>(
            std::max<double>(delete_count, 0.0), len - actual_start));
  } else {
    actual_delete_count = std::min<uint32_t>(0, len - actual_start);
  }
  for (uint32_t k = 0; k < actual_delete_count; ++k) {
    const uint32_t from = actual_start + k;
    if (obj->HasPropertyWithIndex(ctx, from)) {
      const JSVal from_val = obj->GetWithIndex(ctx, from, ERROR(e));
      ary->DefineOwnPropertyWithIndex(
          ctx,
          k,
          DataDescriptor(from_val,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, ERROR(e));
    }
  }

  const uint32_t item_count = (args_size > 2)? args_size - 2 : 0;
  if (item_count < actual_delete_count) {
    for (uint32_t k = actual_start,
         last = len - actual_delete_count; k < last; ++k) {
      const uint32_t from = k + actual_delete_count;
      const uint32_t to = k + item_count;
      if (obj->HasPropertyWithIndex(ctx, from)) {
        const JSVal from_value = obj->GetWithIndex(ctx, from, ERROR(e));
        obj->PutWithIndex(ctx, to, from_value, true, ERROR(e));
      } else {
        obj->DeleteWithIndex(ctx, to, true, ERROR(e));
      }
    }
    for (uint32_t k = len, last = len + item_count - actual_delete_count;
         k > last; --k) {
        obj->DeleteWithIndex(ctx, (k - 1), true, ERROR(e));
    }
  } else if (item_count > actual_delete_count) {
    for (uint32_t k = len - actual_delete_count; actual_start < k; --k) {
      const uint32_t from = k + actual_delete_count - 1;
      const uint32_t to = k + item_count - 1;
      if (obj->HasPropertyWithIndex(ctx, from)) {
        const JSVal from_value = obj->GetWithIndex(ctx, from, ERROR(e));
        obj->PutWithIndex(ctx, to, from_value, true, ERROR(e));
      } else {
        obj->DeleteWithIndex(ctx, to, true, ERROR(e));
      }
    }
  }
  Arguments::const_iterator it = args.begin();
  std::advance(it, 2);
  for (uint32_t k = 0; k < item_count ; ++k, ++it) {
    obj->PutWithIndex(
        ctx,
        k + actual_start,
        *it, true, ERROR(e));
  }
  obj->Put(
      ctx,
      ctx->length_symbol(),
      len - actual_delete_count + item_count, true, ERROR(e));
  return ary;
}

// section 15.4.4.13 Array.prototype.unshift([item1[, item2[, ...]]])
inline JSVal ArrayUnshift(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.unshift", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  for (uint32_t k = len; k > 0; --k) {
    const uint32_t from = k - 1;
    const uint32_t to = k + arg_count - 1;
    if (obj->HasPropertyWithIndex(ctx, from)) {
      const JSVal from_value = obj->GetWithIndex(ctx, from, ERROR(e));
      obj->PutWithIndex(ctx, to, from_value, true, ERROR(e));
    } else {
      obj->DeleteWithIndex(ctx, to, true, ERROR(e));
    }
  }

  uint32_t j = 0;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it, ++j) {
    obj->PutWithIndex(
        ctx,
        j,
        *it,
        true, ERROR(e));
  }
  obj->Put(
      ctx,
      ctx->length_symbol(),
      len + arg_count,
      true, ERROR(e));
  return len + arg_count;
}

// section 15.4.4.14 Array.prototype.indexOf(searchElement[, fromIndex])
inline JSVal ArrayIndexOf(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.indexOf", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  if (len == 0) {
    return -1;
  }

  JSVal search_element;
  if (arg_count > 0) {
    search_element = args[0];
  } else {
    search_element = JSUndefined;
  }

  uint32_t k;
  if (arg_count > 1) {
    double fromIndex = args[1].ToNumber(ctx, ERROR(e));
    fromIndex = core::DoubleToInteger(fromIndex);
    if (fromIndex >= len) {
      return -1;
    }
    if (fromIndex >= 0) {
      k = core::DoubleToUInt32(fromIndex);
    } else {
      if (len < -fromIndex) {
        k = 0;
      } else {
        k = len - core::DoubleToUInt32(-fromIndex);
      }
    }
  } else {
    k = 0;
  }

  for (; k < len; ++k) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      const JSVal element_k = obj->GetWithIndex(ctx, k, ERROR(e));
      if (StrictEqual(search_element, element_k)) {
        return k;
      }
    }
  }
  return -1;
}

// section 15.4.4.15 Array.prototype.lastIndexOf(searchElement[, fromIndex])
inline JSVal ArrayLastIndexOf(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.lastIndexOf", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  if (len == 0) {
    return -1;
  }

  JSVal search_element;
  if (arg_count > 0) {
    search_element = args[0];
  } else {
    search_element = JSUndefined;
  }

  uint32_t k;
  if (arg_count > 1) {
    double fromIndex = args[1].ToNumber(ctx, ERROR(e));
    fromIndex = core::DoubleToInteger(fromIndex);
    if (fromIndex >= 0) {
      if (fromIndex > (len - 1)) {
        k = len - 1;
      } else {
        k = core::DoubleToUInt32(fromIndex);
      }
    } else {
      if (-fromIndex > len) {
        return -1;
      } else {
        k = len - core::DoubleToUInt32(-fromIndex);
      }
    }
  } else {
    k = len - 1;
  }

  while (true) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      const JSVal element_k = obj->GetWithIndex(ctx, k, ERROR(e));
      if (StrictEqual(search_element, element_k)) {
        return k;
      }
    }
    if (k == 0) {
      break;
    } else {
      k--;
    }
  }
  return -1;
}

// section 15.4.4.16 Array.prototype.every(callbackfn[, thisArg])
inline JSVal ArrayEvery(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.every", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  if (arg_count == 0 || !args[0].IsCallable()) {
    e->Report(
        Error::Type,
        "Array.protoype.every requires callable object as 1st argument");
    return JSUndefined;
  }
  JSFunction* const callbackfn = args[0].object()->AsCallable();

  Arguments arg_list(ctx, 3, ERROR(e));
  const JSVal this_binding = (arg_count > 1) ? args[1] : JSUndefined;
  arg_list[2] = obj;

  for (uint32_t k = 0; k < len; ++k) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      arg_list[0] = obj->GetWithIndex(ctx, k, ERROR(e));
      arg_list[1] = k;
      const JSVal test_result = callbackfn->Call(arg_list,
                                                 this_binding,
                                                 ERROR(e));
      const bool result = test_result.ToBoolean(ERROR(e));
      if (!result) {
        return JSFalse;
      }
    }
  }
  return JSTrue;
}

// section 15.4.4.17 Array.prototype.some(callbackfn[, thisArg])
inline JSVal ArraySome(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.some", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  if (arg_count == 0 || !args[0].IsCallable()) {
    e->Report(
        Error::Type,
        "Array.protoype.some requires callable object as 1st argument");
    return JSUndefined;
  }
  JSFunction* const callbackfn = args[0].object()->AsCallable();

  Arguments arg_list(ctx, 3, ERROR(e));
  const JSVal this_binding = (arg_count > 1) ? args[1] : JSUndefined;
  arg_list[2] = obj;

  for (uint32_t k = 0; k < len; ++k) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      arg_list[0] = obj->GetWithIndex(ctx, k, ERROR(e));
      arg_list[1] = k;
      const JSVal test_result = callbackfn->Call(arg_list,
                                                 this_binding,
                                                 ERROR(e));
      const bool result = test_result.ToBoolean(ERROR(e));
      if (result) {
        return JSTrue;
      }
    }
  }
  return JSFalse;
}

// section 15.4.4.18 Array.prototype.forEach(callbackfn[, thisArg])
inline JSVal ArrayForEach(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.forEach", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  if (arg_count == 0 || !args[0].IsCallable()) {
    e->Report(
        Error::Type,
        "Array.protoype.forEach requires callable object as 1st argument");
    return JSUndefined;
  }
  JSFunction* const callbackfn = args[0].object()->AsCallable();

  Arguments arg_list(ctx, 3, ERROR(e));
  const JSVal this_binding = (arg_count > 1) ? args[1] : JSUndefined;
  arg_list[2] = obj;

  for (uint32_t k = 0; k < len; ++k) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      arg_list[0] = obj->GetWithIndex(ctx, k, ERROR(e));
      arg_list[1] = k;
      callbackfn->Call(arg_list, this_binding, ERROR(e));
    }
  }
  return JSUndefined;
}

// section 15.4.4.19 Array.prototype.map(callbackfn[, thisArg])
inline JSVal ArrayMap(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.map", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  if (arg_count == 0 || !args[0].IsCallable()) {
    e->Report(
        Error::Type,
        "Array.protoype.map requires callable object as 1st argument");
    return JSUndefined;
  }
  JSFunction* const callbackfn = args[0].object()->AsCallable();

  JSArray* const ary = JSArray::New(ctx, len);

  Arguments arg_list(ctx, 3, ERROR(e));
  const JSVal this_binding = (arg_count > 1) ? args[1] : JSUndefined;
  arg_list[2] = obj;

  for (uint32_t k = 0; k < len; ++k) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      arg_list[0] = obj->GetWithIndex(ctx, k, ERROR(e));
      arg_list[1] = k;
      const JSVal mapped_value = callbackfn->Call(arg_list,
                                                  this_binding,
                                                  ERROR(e));
      ary->DefineOwnPropertyWithIndex(
          ctx,
          k,
          DataDescriptor(mapped_value,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, ERROR(e));
    }
  }
  return ary;
}

// section 15.4.4.20 Array.prototype.filter(callbackfn[, thisArg])
inline JSVal ArrayFilter(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.filter", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  if (arg_count == 0 || !args[0].IsCallable()) {
    e->Report(
        Error::Type,
        "Array.protoype.filter requires callable object as 1st argument");
    return JSUndefined;
  }
  JSFunction* const callbackfn = args[0].object()->AsCallable();

  JSArray* const ary = JSArray::New(ctx);

  Arguments arg_list(ctx, 3, ERROR(e));
  const JSVal this_binding = (arg_count > 1) ? args[1] : JSUndefined;
  arg_list[2] = obj;

  for (uint32_t k = 0, to = 0; k < len; ++k) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      const JSVal k_value = obj->GetWithIndex(ctx, k, ERROR(e));
      arg_list[0] = k_value;
      arg_list[1] = k;
      const JSVal selected = callbackfn->Call(arg_list,
                                              this_binding,
                                              ERROR(e));
      const bool result = selected.ToBoolean(ERROR(e));
      if (result) {
        ary->DefineOwnPropertyWithIndex(
            ctx,
            to,
            DataDescriptor(k_value,
                           PropertyDescriptor::WRITABLE |
                           PropertyDescriptor::ENUMERABLE |
                           PropertyDescriptor::CONFIGURABLE),
            false, ERROR(e));
        ++to;
      }
    }
  }
  return ary;
}

// section 15.4.4.21 Array.prototype.reduce(callbackfn[, initialValue])
inline JSVal ArrayReduce(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.reduce", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  if (arg_count == 0 || !args[0].IsCallable()) {
    e->Report(
        Error::Type,
        "Array.protoype.reduce requires callable object as 1st argument");
    return JSUndefined;
  }
  JSFunction* const callbackfn = args[0].object()->AsCallable();

  if (len == 0 && arg_count <= 1) {
    e->Report(
        Error::Type,
        "Array.protoype.reduce with empty Array requires "
        "initial value as 2nd argument");
    return JSUndefined;
  }

  uint32_t k = 0;
  JSVal accumulator;
  if (arg_count > 1) {
    accumulator = args[1];
  } else {
    bool k_present = false;
    for (; k < len; ++k) {
      if (obj->HasPropertyWithIndex(ctx, k)) {
        k_present = true;
        accumulator = obj->GetWithIndex(ctx, k, ERROR(e));
        ++k;
        break;
      }
    }
    if (!k_present) {
      e->Report(
          Error::Type,
          "Array.protoype.reduce with empty Array requires initial value");
      return JSUndefined;
    }
  }

  Arguments arg_list(ctx, 4, ERROR(e));
  arg_list[3] = obj;

  for (;k < len; ++k) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      arg_list[0] = accumulator;
      arg_list[1] = obj->GetWithIndex(ctx, k, ERROR(e));
      arg_list[2] = k;
      accumulator = callbackfn->Call(arg_list,
                                     JSUndefined,
                                     ERROR(e));
    }
  }
  return accumulator;
}

// section 15.4.4.22 Array.prototype.reduceRight(callbackfn[, initialValue])
inline JSVal ArrayReduceRight(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("Array.prototype.reduceRight", args, e);
  Context* const ctx = args.ctx();
  JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(e));
  const JSVal length = obj->Get(
      ctx,
      ctx->length_symbol(), ERROR(e));
  const double val = length.ToNumber(ctx, ERROR(e));
  const uint32_t arg_count = args.size();
  const uint32_t len = core::DoubleToUInt32(val);

  if (arg_count == 0 || !args[0].IsCallable()) {
    e->Report(
        Error::Type,
        "Array.protoype.reduceRight requires "
        "callable object as 1st argument");
    return JSUndefined;
  }
  JSFunction* const callbackfn = args[0].object()->AsCallable();

  if (len == 0 && arg_count <= 1) {
    e->Report(
        Error::Type,
        "Array.protoype.reduceRight with empty Array requires "
        "initial value as 2nd argument");
    return JSUndefined;
  }

  uint32_t k = len;
  JSVal accumulator;
  if (arg_count > 1) {
    accumulator = args[1];
  } else {
    bool k_present = false;
    while (k--) {
      if (obj->HasPropertyWithIndex(ctx, k)) {
        k_present = true;
        accumulator = obj->GetWithIndex(ctx, k, ERROR(e));
        break;
      }
    }
    if (!k_present) {
      e->Report(
          Error::Type,
          "Array.protoype.reduceRight with empty Array requires initial value");
      return JSUndefined;
    }
  }

  Arguments arg_list(ctx, 4, ERROR(e));
  arg_list[3] = obj;

  while (k--) {
    if (obj->HasPropertyWithIndex(ctx, k)) {
      arg_list[0] = accumulator;
      arg_list[1] = obj->GetWithIndex(ctx, k, ERROR(e));
      arg_list[2] = k;
      accumulator = callbackfn->Call(arg_list,
                                     JSUndefined,
                                     ERROR(e));
    }
  }
  return accumulator;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_ARRAY_H_
