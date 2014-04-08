#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jssymbol.h>
#include <iv/lv5/jssymbolobject.h>
#include <iv/lv5/jsstring_builder.h>
#include <iv/lv5/runtime/symbol.h>
namespace iv {
namespace lv5 {
namespace runtime {

JSVal SymbolConstructor(const Arguments& args, Error* e) {
  Context* ctx = args.ctx();
  if (args.IsConstructorCalled()) {
    return SymbolCreate(args, e);
  }
  JSVal description = args.At(0);
  if (!description.IsUndefined()) {
    description = description.ToString(ctx, IV_LV5_ERROR(e));
  }
  return JSSymbol::New(ctx, description);
}

JSVal SymbolToString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Symbol.toString", args, e);
  Context* ctx = args.ctx();
  const JSVal s = args.this_binding();
  if (!s.IsObject() || !s.object()->IsClass<Class::Symbol>()) {
    e->Report(Error::Type, "Cannot apply Symbol.toString to non-Symbol object");
    return JSEmpty;
  }

  JSSymbol* sym = static_cast<JSSymbolObject*>(s.object())->symbol();
  JSVal desc = sym->description();
  if (desc.IsUndefined()) {
    desc = JSString::NewEmpty(ctx);
  }
  assert(desc.IsString());

  JSStringBuilder builder;
  builder.Append("Symbol(");
  builder.AppendJSString(*desc.string());
  builder.Append(")");
  return builder.Build(ctx, false, e);
}

JSVal SymbolValueOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Symbol.valueOf", args, e);
  Context* ctx = args.ctx();
  const JSVal s = args.this_binding();
  if (!s.IsObject() || !s.object()->IsClass<Class::Symbol>()) {
    e->Report(Error::Type, "Cannot apply Symbol.valueOf to non-Symbol object");
    return JSEmpty;
  }

  return static_cast<JSSymbolObject*>(s.object())->symbol();
}

JSVal SymbolCreate(const Arguments& args, Error* e) {
  e->Report(Error::Type, "Symbol constructor cannot be called as constructor");
  return JSEmpty;
}

JSVal SymbolToPrimitive(const Arguments& args, Error* e) {
  e->Report(Error::Type, "Symbol toPrimitive should be failed");
  return JSEmpty;
}

} } }  // namespace iv::lv5::runtime
