#include <iostream>  // NOLINT
#include "lv5.h"
#include "error.h"
#include "runtime.h"
#include "context.h"
namespace iv {
namespace lv5 {
namespace {

const std::string function_prefix("function ");
const std::string error_split(": ");
const double kNaN = std::numeric_limits<double>::quiet_NaN();
const double kInfinity = std::numeric_limits<double>::infinity();

}  // namespace

JSVal Runtime_ThrowTypeError(const Arguments& args, Error* error) {
  error->Report(Error::Type,
                "[[ThrowTypeError]] called");
  return JSUndefined;
}

JSVal Runtime_ObjectConstructor(const Arguments& args,
                                Error* error) {
  if (args.size() == 1) {
    const JSVal& val = args[0];
    if (val.IsNull() || val.IsUndefined()) {
      return JSObject::New(args.ctx());
    } else {
      JSObject* const obj = val.ToObject(args.ctx(), ERROR(error));
      return obj;
    }
  } else {
    return JSObject::New(args.ctx());
  }
}

JSVal Runtime_ObjectHasOwnProperty(const Arguments& args,
                                   Error* error) {
  if (args.size() > 0) {
    const JSVal& val = args[0];
    Context* ctx = args.ctx();
    JSString* const str = val.ToString(ctx, ERROR(error));
    JSObject* const obj = args.this_binding().ToObject(ctx, ERROR(error));
    if (!obj->GetOwnProperty(ctx->Intern(*str)).IsEmpty()) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSFalse;
  }
}

JSVal Runtime_ObjectToString(const Arguments& args, Error* error) {
  std::string ascii;
  JSObject* const obj = args.this_binding().ToObject(args.ctx(), ERROR(error));
  JSString* const cls = obj->cls();
  assert(cls);
  std::string str("[object ");
  str.append(cls->begin(), cls->end());
  str.append("]");
  return JSString::NewAsciiString(args.ctx(), str.c_str());
}

JSVal Runtime_FunctionToString(const Arguments& args,
                               Error* error) {
  const JSVal& obj = args.this_binding();
  if (obj.IsCallable()) {
    JSFunction* const func = obj.object()->AsCallable();
    if (func->AsNativeFunction()) {
      return JSString::NewAsciiString(args.ctx(),
                                      "function () { [native code] }");
    } else {
      core::UString buffer(function_prefix.begin(),
                           function_prefix.end());
      if (func->AsCodeFunction()->name()) {
        const core::UStringPiece name = func->AsCodeFunction()->name()->value();
        buffer.append(name.data(), name.size());
      }
      const core::UStringPiece src = func->AsCodeFunction()->GetSource();
      buffer.append(src.data(), src.size());
      return JSString::New(args.ctx(), buffer);
    }
  }
  error->Report(Error::Type,
                "Function.prototype.toString is not generic function");
  return JSUndefined;
}

JSVal Runtime_ErrorToString(const Arguments& args, Error* error) {
  const JSVal& obj = args.this_binding();
  Context* ctx = args.ctx();
  if (obj.IsObject()) {
    JSString* name;
    {
      const JSVal target = obj.object()->Get(ctx,
                                             ctx->Intern("name"),
                                             ERROR(error));
      if (target.IsUndefined()) {
        name = JSString::NewAsciiString(ctx, "Error");
      } else {
        name = target.ToString(ctx, ERROR(error));
      }
    }
    JSString* msg;
    {
      const JSVal target = obj.object()->Get(ctx,
                                             ctx->Intern("message"),
                                             ERROR(error));
      if (target.IsUndefined()) {
        return JSUndefined;
      } else {
        msg = target.ToString(ctx, ERROR(error));
      }
    }
    core::UString buffer;
    buffer.append(name->data(), name->size());
    buffer.append(error_split.begin(), error_split.end());
    buffer.append(msg->data(), msg->size());
    return JSString::New(ctx, buffer);
  }
  error->Report(Error::Type, "base must be object");
  return JSUndefined;
}

JSVal Runtime_MathAbs(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::abs(x);
  }
  return kNaN;
}

JSVal Runtime_MathAcos(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::acos(x);
  }
  return kNaN;
}

JSVal Runtime_MathAsin(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::asin(x);
  }
  return kNaN;
}

JSVal Runtime_MathAtan(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::atan(x);
  }
  return kNaN;
}

JSVal Runtime_MathAtan2(const Arguments& args, Error* error) {
  if (args.size() > 1) {
    const double y = args[0].ToNumber(args.ctx(), ERROR(error));
    const double x = args[1].ToNumber(args.ctx(), ERROR(error));
    return std::atan2(y, x);
  }
  return kNaN;
}

JSVal Runtime_MathCeil(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::ceil(x);
  }
  return kNaN;
}

JSVal Runtime_MathCos(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::cos(x);
  }
  return kNaN;
}

JSVal Runtime_MathExp(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::exp(x);
  }
  return kNaN;
}

JSVal Runtime_MathFloor(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::floor(x);
  }
  return kNaN;
}

JSVal Runtime_MathLog(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::log(x);
  }
  return kNaN;
}

JSVal Runtime_MathMax(const Arguments& args, Error* error) {
  double max = -kInfinity;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const double x = it->ToNumber(args.ctx(), ERROR(error));
    if (std::isnan(x)) {
      return x;
    } else if (x > max) {
      max = x;
    }
  }
  return max;
}

JSVal Runtime_MathMin(const Arguments& args, Error* error) {
  double min = kInfinity;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const double x = it->ToNumber(args.ctx(), ERROR(error));
    if (std::isnan(x)) {
      return x;
    } else if (x < min) {
      min = x;
    }
  }
  return min;
}

JSVal Runtime_MathPow(const Arguments& args, Error* error) {
  if (args.size() > 1) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    const double y = args[1].ToNumber(args.ctx(), ERROR(error));
    return std::pow(x, y);
  }
  return kNaN;
}

JSVal Runtime_MathRandom(const Arguments& args, Error* error) {
  return args.ctx()->Random();
}

JSVal Runtime_MathRound(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    const double res = std::ceil(x);
    if (res - x > 0.5) {
      return res - 1;
    } else {
      return res;
    }
  }
  return kNaN;
}

JSVal Runtime_MathSin(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::sin(x);
  }
  return kNaN;
}

JSVal Runtime_MathSqrt(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::sqrt(x);
  }
  return kNaN;
}

JSVal Runtime_MathTan(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const double x = args[0].ToNumber(args.ctx(), ERROR(error));
    return std::tan(x);
  }
  return kNaN;
}

} }  // namespace iv::lv5
