#include <iostream>  // NOLINT
#include <gc/gc.h>  // NOLINT
#include "lv5.h"
#include "error.h"
#include "jserror.h"
#include "runtime.h"
#include "context.h"
#include "eval-source.h"
#include "parser.h"
namespace iv {
namespace lv5 {
namespace {

const std::string function_prefix("function ");
const std::string error_split(": ");
const double kNaN = std::numeric_limits<double>::quiet_NaN();
const double kInfinity = std::numeric_limits<double>::infinity();

static JSString* ErrorMessageString(const Arguments& args, Error* error) {
  if (args.size() > 0) {
    const JSVal& msg = args[0];
    if (!msg.IsUndefined()) {
      return msg.ToString(args.ctx(), ERROR_WITH(error, NULL));
    } else {
      return JSString::NewEmptyString(args.ctx());
    }
  } else {
    return JSString::NewEmptyString(args.ctx());
  }
}

static JSScript* CompileScript(Context* ctx, JSString* str, Error* error) {
  EvalSource* const src = new EvalSource(str);
  AstFactory* const factory = new AstFactory(ctx);
  core::Parser<AstFactory> parser(src, factory);
  parser.set_strict(ctx->IsStrict());
  const iv::lv5::FunctionLiteral* const eval = parser.ParseProgram();
  if (!eval) {
    delete src;
    delete factory;
    error->Report(Error::Syntax,
                  parser.error());
    return NULL;
  } else {
    return iv::lv5::JSScript::NewEval(ctx, eval, factory, src);
  }
}

}  // namespace

JSVal Runtime_GlobalEval(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    error->Report(Error::Type,
                  "function eval() { [native code] } is not a constructor");
    return JSUndefined;
  } else {
    return Runtime_InDirectCallToEval(args, error);
  }
}

JSVal Runtime_DirectCallToEval(const Arguments& args, Error* error) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = args.ctx();
  JSScript* const script = CompileScript(args.ctx(), first.string(), ERROR(error));
  if (script->function()->strict()) {
    JSDeclEnv* const env = Interpreter::NewDeclarativeEnvironment(ctx, ctx->lexical_env());
    const Interpreter::ContextSwitcher switcher(ctx,
                                                env,
                                                env,
                                                ctx->this_binding(),
                                                true);
    ctx->Run(script);
  } else {
    const Interpreter::ContextSwitcher switcher(ctx,
                                                ctx->global_env(),
                                                ctx->global_env(),
                                                ctx->global_obj(),
                                                false);
    ctx->Run(script);
  }
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

JSVal Runtime_InDirectCallToEval(const Arguments& args, Error* error) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = args.ctx();
  JSScript* const script = CompileScript(args.ctx(), first.string(), ERROR(error));
  if (script->function()->strict()) {
    JSDeclEnv* const env = Interpreter::NewDeclarativeEnvironment(ctx, ctx->lexical_env());
    const Interpreter::ContextSwitcher switcher(ctx,
                                                env,
                                                env,
                                                ctx->this_binding(),
                                                true);
    ctx->Run(script);
  } else {
    ctx->Run(script);
  }
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

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

JSVal Runtime_ErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::New(args.ctx(), Error::User, message);
}

JSVal Runtime_NativeErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewSyntaxError(args.ctx(), message);
}

JSVal Runtime_EvalErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewEvalError(args.ctx(), message);
}

JSVal Runtime_RangeErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewRangeError(args.ctx(), message);
}

JSVal Runtime_ReferenceErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewReferenceError(args.ctx(), message);
}

JSVal Runtime_SyntaxErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewSyntaxError(args.ctx(), message);
}

JSVal Runtime_TypeErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewTypeError(args.ctx(), message);
}

JSVal Runtime_URIErrorConstructor(const Arguments& args, Error* error) {
  JSString* message = ErrorMessageString(args, ERROR(error));
  return JSError::NewURIError(args.ctx(), message);
}

JSVal Runtime_ErrorToString(const Arguments& args, Error* error) {
  const JSVal& obj = args.this_binding();
  Context* const ctx = args.ctx();
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

JSVal Runtime_NumberConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    double res = 0.0;
    if (args.size() > 0) {
      res = args[0].ToNumber(args.ctx(), ERROR(error));
    }
    return JSNumberObject::New(args.ctx(), res);
  } else {
    if (args.size() > 0) {
      return args[0].ToNumber(args.ctx(), error);
    } else {
      return 0.0;
    }
  }
}

JSVal Runtime_NumberToString(const Arguments& args, Error* error) {
  const JSVal& obj = args.this_binding();
  double num;
  if (!obj.IsNumber()) {
    if (obj.IsObject() && obj.object()->AsNumberObject()) {
      num = obj.object()->AsNumberObject()->value();
    } else {
      error->Report(Error::Type,
                    "Number.prototype.toString is not generic function");
      return JSUndefined;
    }
  } else {
    num = obj.number();
  }
  if (args.size() > 0) {
    const JSVal& first = args[0];
    double radix = first.ToNumber(args.ctx(), ERROR(error));
    radix = core::DoubleToInteger(radix);
    if (2 <= radix && radix <= 36) {
      // if radix == 10, through to radix 10 or no radix
      if (radix != 10) {
        std::string buffer;
        core::DoubleToStringWithRadix(
            num,
            static_cast<int>(radix),
            &buffer);
        return JSString::NewAsciiString(args.ctx(), buffer);
      }
    } else {
      // TODO(Constellation) more details
      error->Report(Error::Range,
                    "inllegal radix");
      return JSUndefined;
    }
  }
  // radix 10 or no radix
  std::tr1::array<char, 80> buffer;
  const char* const str = core::DoubleToCString(num,
                                                buffer.data(),
                                                buffer.size());
  return JSString::NewAsciiString(args.ctx(), str);
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
