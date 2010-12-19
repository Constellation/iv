#include <limits>
#include <algorithm>
#include <tr1/array>
#include <cstdlib>
#include "conversions.h"
#include "chars.h"
#include "ustringpiece.h"
#include "jsarray.h"
#include "jsobject.h"
#include "property.h"
#include "jsstring.h"
#include "context.h"
#include "class.h"

namespace iv {
namespace lv5 {

JSArray::JSArray(Context* ctx, std::size_t len)
  : JSObject(),
    length_(len) {
  JSObject::DefineOwnProperty(ctx, ctx->length_symbol(),
                              DataDescriptor(len,
                                             PropertyDescriptor::WRITABLE),
                                             false, ctx->error());
}

#define REJECT(str)\
  do {\
    if (th) {\
      res->Report(Error::Type, str);\
    }\
    return false;\
  } while (0)

bool JSArray::DefineOwnProperty(Context* ctx,
                                Symbol name,
                                const PropertyDescriptor& desc,
                                bool th,
                                Error* res) {
  const Symbol length_symbol = ctx->length_symbol();
  DataDescriptor* const old_len_desc =
      GetOwnProperty(ctx, length_symbol).AsDataDescriptor();

  const JSVal& len_value = old_len_desc->value();

  if (name == length_symbol) {
    if (desc.IsDataDescriptor()) {
      const DataDescriptor* const data = desc.AsDataDescriptor();
      if (data->IsValueAbsent()) {
        return JSObject::DefineOwnProperty(ctx, name, desc, th, res);
      }
      DataDescriptor new_len_desc(*data);
      const double new_len_double = new_len_desc.value().ToNumber(ctx, res);
      if (*res) {
        return false;
      }
      const uint32_t new_len = core::DoubleToUInt32(new_len_double);
      if (new_len != new_len_double) {
        res->Report(Error::Range, "out of range");
        return false;
      }
      new_len_desc.set_value(new_len);
      double old_len = len_value.ToNumber(ctx, res);
      if (*res) {
        return false;
      }
      if (new_len >= old_len) {
        return JSObject::DefineOwnProperty(ctx, length_symbol,
                                           new_len_desc, th, res);
      }
      if (!old_len_desc->IsWritable()) {
        REJECT("\"length\" not writable");
      }
      const bool new_writable =
          new_len_desc.IsWritableAbsent() || new_len_desc.IsWritable();
      new_len_desc.set_writable(true);
      const bool succeeded = JSObject::DefineOwnProperty(ctx, length_symbol,
                                                         new_len_desc, th, res);
      if (!succeeded) {
        return false;
      }
      std::tr1::array<char, 80> buffer;
      while (new_len < old_len) {
        old_len -= 1;
        const char* const str = core::DoubleToCString(old_len,
                                                      buffer.data(),
                                                      buffer.size());
        const Symbol now_index = ctx->Intern(str);
        // see Eratta
        const bool delete_succeeded = Delete(ctx, now_index, false, res);
        if (*res) {
          return false;
        }
        if (!delete_succeeded) {
          new_len_desc.set_value(old_len + 1);
          if (!new_writable) {
            new_len_desc.set_writable(false);
          }
          JSObject::DefineOwnProperty(ctx, length_symbol,
                                      new_len_desc, false, res);
          if (*res) {
            return false;
          }
          REJECT("shrink array failed");
        }
      }
      if (!new_writable) {
        JSObject::DefineOwnProperty(ctx, length_symbol,
                                    DataDescriptor(
                                        PropertyDescriptor::WRITABLE |
                                        PropertyDescriptor::UNDEF_ENUMERABLE |
                                        PropertyDescriptor::UNDEF_CONFIGURABLE),
                                    false, res);
      }
      return true;
    } else {
      UNREACHABLE();
      return true;
    }
  } else {
    uint32_t index;
    if (core::ConvertToUInt32(ctx->GetContent(name), &index)) {
      // array index
      const double old_len = len_value.ToNumber(ctx, res);
      if (*res) {
        return false;
      }
      if (index >= old_len && !old_len_desc->IsWritable()) {
        return false;
      }
      const bool succeeded = JSObject::DefineOwnProperty(ctx, name,
                                                         desc, false, res);
      if (*res) {
        return false;
      }
      if (!succeeded) {
        REJECT("define own property failed");
      }
      if (index >= old_len) {
        old_len_desc->set_value(index+1);
        JSObject::DefineOwnProperty(ctx, length_symbol,
                                    *old_len_desc, false, res);
      }
      return true;
    }
    return JSObject::DefineOwnProperty(ctx, name, desc, th, res);
  }
}

#undef REJECT

JSArray* JSArray::New(Context* ctx) {
  JSArray* const ary = new JSArray(ctx, 0);
  const Class& cls = ctx->Cls("Array");
  ary->set_class_name(cls.name);
  ary->set_prototype(cls.prototype);
  return ary;
}

JSArray* JSArray::New(Context* ctx, std::size_t n) {
  JSArray* const ary = new JSArray(ctx, n);
  const Class& cls = ctx->Cls("Array");
  ary->set_class_name(cls.name);
  ary->set_prototype(cls.prototype);
  return ary;
}

} }  // namespace iv::lv5
