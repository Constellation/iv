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
                              DataDescriptor(0.0,
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

// TODO(Constellation) res => NULL safe
bool JSArray::DefineOwnProperty(Context* ctx,
                                Symbol name,
                                const PropertyDescriptor& desc,
                                bool th,
                                Error* res) {
  const Symbol length_symbol = ctx->length_symbol();
  PropertyDescriptor old_len_desc_upper = GetOwnProperty(length_symbol);
  assert(!old_len_desc_upper.IsEmpty() &&
         old_len_desc_upper.IsDataDescriptor());
  DataDescriptor* const old_len_desc = old_len_desc_upper.AsDataDescriptor();

  const JSVal& len_value = old_len_desc->data();
  const core::UString& name_string = ctx->GetContent(name);

  if (name == length_symbol) {
    if (desc.IsDataDescriptor()) {
      const JSVal& new_len_value = desc.AsDataDescriptor()->data();
      const double new_len_double = new_len_value.ToNumber(ctx, res);
      if (*res) {
        return false;
      }
      uint32_t new_len = core::DoubleToUInt32(new_len_double);
      if (new_len != new_len_double) {
        res->Report(Error::Range, "out of range");
        return false;
      }
      double old_len = len_value.ToNumber(ctx, res);
      if (*res) {
        return false;
      }
      DataDescriptor new_len_desc(new_len, desc.attrs());
      if (new_len >= old_len) {
        return JSObject::DefineOwnProperty(ctx, length_symbol,
                                           new_len_desc, th, res);
      }
      if (!old_len_desc->IsWritable()) {
        REJECT("\"length\" not writable");
      }
      const bool new_writable =
          new_len_desc.IsWritableAbsent() || new_len_desc.IsWritable();
      if (!new_writable) {
        new_len_desc.SetWritable(true);
      }
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
        const bool delete_succeeded = Delete(now_index, false, res);
        if (*res) {
          return false;
        }
        if (!delete_succeeded) {
          new_len_desc.set_value(old_len + 1);
          if (!new_writable) {
            new_len_desc.SetWritable(false);
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
        new_len_desc.SetWritable(false);
        JSObject::DefineOwnProperty(ctx, length_symbol,
                                    new_len_desc, false, res);
      }
      return true;
    } else {
      UNREACHABLE();
      return true;
    }
  } else {
    uint32_t index;
    if (core::ConvertToUInt32(name_string, &index)) {
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
  ary->set_cls(cls.name);
  ary->set_prototype(cls.prototype);
  return ary;
}

JSArray* JSArray::New(Context* ctx, std::size_t n) {
  JSArray* const ary = new JSArray(ctx, n);
  const Class& cls = ctx->Cls("Array");
  ary->set_cls(cls.name);
  ary->set_prototype(cls.prototype);
  return ary;
}

} }  // namespace iv::lv5
