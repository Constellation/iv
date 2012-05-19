#include <cmath>
#include <cstdio>
#include <cstring>
#include <iv/detail/array.h>
#include <iv/ustring.h>
#include <iv/dtoa.h>
#include <iv/canonicalized_nan.h>
#include <iv/lv5/jsmath.h>
#include <iv/lv5/jsenv.h>
#include <iv/lv5/jsjson.h>
#include <iv/lv5/jsmap.h>
#include <iv/lv5/jsset.h>
#include <iv/lv5/jsglobal.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/jsarguments.h>
#include <iv/lv5/jsnumberobject.h>
#include <iv/lv5/jsbooleanobject.h>
#include <iv/lv5/jsi18n.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/context.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/property.h>
#include <iv/lv5/class.h>
#include <iv/lv5/runtime.h>
#include <iv/lv5/jserror.h>
#include <iv/lv5/bind.h>
#include <iv/lv5/internal.h>
#include <iv/lv5/radio/radio.h>
namespace iv {
namespace lv5 {
namespace context {

const ClassSlot& GetClassSlot(const Context* ctx, Class::JSClassType type) {
  return ctx->global_data()->GetClassSlot(type);
}

Symbol Intern(Context* ctx, const core::StringPiece& str) {
  return ctx->global_data()->Intern(str);
}

Symbol Intern(Context* ctx, const core::UStringPiece& str) {
  return ctx->global_data()->Intern(str);
}

Symbol Intern(Context* ctx, const JSString* str) {
  if (str->Is8Bit()) {
    return ctx->global_data()->Intern(*str->Get8Bit());
  } else {
    return ctx->global_data()->Intern(*str->Get16Bit());
  }
}

Symbol Intern(Context* ctx, uint32_t index) {
  return ctx->global_data()->InternUInt32(index);
}

Symbol Intern(Context* ctx, double number) {
  return ctx->global_data()->InternDouble(number);
}

Symbol Intern64(Context* ctx, uint64_t number) {
  return ctx->global_data()->Intern64(number);
}

GlobalData* Global(Context* ctx) {
  return ctx->global_data();
}

Map* GetEmptyObjectMap(Context* ctx) {
  return ctx->global_data()->GetEmptyObjectMap();
}

Map* GetFunctionMap(Context* ctx) {
  return ctx->global_data()->GetFunctionMap();
}

Map* GetArrayMap(Context* ctx) {
  return ctx->global_data()->GetArrayMap();
}

Map* GetStringMap(Context* ctx) {
  return ctx->global_data()->GetStringMap();
}

Map* GetBooleanMap(Context* ctx) {
  return ctx->global_data()->GetBooleanMap();
}

Map* GetNumberMap(Context* ctx) {
  return ctx->global_data()->GetNumberMap();
}

Map* GetDateMap(Context* ctx) {
  return ctx->global_data()->GetDateMap();
}

Map* GetRegExpMap(Context* ctx) {
  return ctx->global_data()->GetRegExpMap();
}

Map* GetErrorMap(Context* ctx) {
  return ctx->global_data()->GetErrorMap();
}

JSString* EmptyString(Context* ctx) {
  return ctx->global_data()->string_empty();
}

JSString* LookupSingleString(Context* ctx, uint16_t ch) {
  return ctx->global_data()->GetSingleString(ch);
}

JSFunction* throw_type_error(Context* ctx) {
  return ctx->throw_type_error();
}

JSVal* StackGain(Context* ctx, std::size_t size) {
  return ctx->StackGain(size);
}

void StackRestore(Context* ctx, JSVal* ptr) {
  ctx->StackRestore(ptr);
}

void RegisterLiteralRegExp(Context* ctx, JSRegExpImpl* reg) {
  ctx->global_data()->RegisterLiteralRegExp(reg);
}

core::Space* GetRegExpAllocator(Context* ctx) {
  return ctx->regexp_allocator();
}

core::i18n::I18N* I18N(Context* ctx) {
  return ctx->i18n();
}

}  // namespace context

Context::Context(JSAPI fc, JSAPI ge)
  : global_data_(this),
    throw_type_error_(
        JSInlinedFunction<&runtime::ThrowTypeError, 0>::NewPlain(
            this, context::Intern(this, "ThrowError"))),
    global_env_(JSObjectEnv::New(this, NULL, global_obj())),
    regexp_allocator_(),
    regexp_vm_(),
    stack_(NULL),
    function_constructor_(fc),
    global_eval_(ge),
    i18n_() {
  Initialize();
}

void Context::Initialize() {
  JSFunction* func_constructor =
      JSNativeFunction::NewPlain(this, function_constructor_, 1, context::Intern(this, "Function"));
  JSFunction* eval_function =
      JSNativeFunction::NewPlain(this, global_eval_, 1, symbol::eval());

  bind::Object global_binder(this, global_obj());

  // Object and Function
  JSObject* const obj_proto = JSObject::NewPlain(this, Map::NewUniqueMap(this));
  JSFunction* const obj_constructor =
      JSInlinedFunction<&runtime::ObjectConstructor, 1>::NewPlain(
          this,
          context::Intern(this, "Object"));

  // Function
  JSFunction* const func_proto =
      JSInlinedFunction<&runtime::FunctionPrototype, 0>::NewPlain(
          this,
          context::Intern(this, "Function"));

  struct ClassSlot func_cls = {
    JSFunction::GetClass(),
    context::Intern(this, "Function"),
    JSString::NewAsciiString(this, "Function"),
    func_constructor,
    func_proto
  };
  global_data_.RegisterClass<Class::Function>(func_cls);

  struct ClassSlot obj_cls = {
    JSObject::GetClass(),
    context::Intern(this, "Object"),
    JSString::NewAsciiString(this, "Object"),
    obj_constructor,
    obj_proto
  };
  global_data_.RegisterClass<Class::Object>(obj_cls);

  bind::Object(this, func_constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      // seciton 15.3.3.1 Function.prototype
      .def(symbol::prototype(), func_proto, ATTR::NONE);

  bind::Object(this, obj_constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      // seciton 15.2.3.1 Object.prototype
      .def(symbol::prototype(), obj_proto, ATTR::NONE)
      // section 15.2.3.2 Object.getPrototypeOf(O)
      .def<&runtime::ObjectGetPrototypeOf, 1>("getPrototypeOf")
      // section 15.2.3.3 Object.getOwnPropertyDescriptor(O, P)
      .def<
        &runtime::ObjectGetOwnPropertyDescriptor, 2>("getOwnPropertyDescriptor")
      // section 15.2.3.4 Object.getOwnPropertyNames(O)
      .def<&runtime::ObjectGetOwnPropertyNames, 1>("getOwnPropertyNames")
      // section 15.2.3.5 Object.create(O[, Properties])
      .def<&runtime::ObjectCreate, 2>("create")
      // section 15.2.3.6 Object.defineProperty(O, P, Attributes)
      .def<&runtime::ObjectDefineProperty, 3>("defineProperty")
      // section 15.2.3.7 Object.defineProperties(O, Properties)
      .def<&runtime::ObjectDefineProperties, 2>("defineProperties")
      // section 15.2.3.8 Object.seal(O)
      .def<&runtime::ObjectSeal, 1>("seal")
      // section 15.2.3.9 Object.freeze(O)
      .def<&runtime::ObjectFreeze, 1>("freeze")
      // section 15.2.3.10 Object.preventExtensions(O)
      .def<&runtime::ObjectPreventExtensions, 1>("preventExtensions")
      // section 15.2.3.11 Object.isSealed(O)
      .def<&runtime::ObjectIsSealed, 1>("isSealed")
      // section 15.2.3.12 Object.isFrozen(O)
      .def<&runtime::ObjectIsFrozen, 1>("isFrozen")
      // section 15.2.3.13 Object.isExtensible(O)
      .def<&runtime::ObjectIsExtensible, 1>("isExtensible")
      // section 15.2.3.14 Object.keys(O)
      .def<&runtime::ObjectKeys, 1>("keys")
      // section 15.2.3.15 Object.isObject(O)
      .def<&runtime::ObjectIsObject, 1>("isObject")
      // ES.next Object.is(x, y)
      .def<&runtime::ObjectIs, 2>("is");

  bind::Object(this, func_proto)
      .cls(func_cls.cls)
      .prototype(obj_proto)
      // section 15.3.4.1 Function.prototype.constructor
      .def(symbol::constructor(), func_constructor, ATTR::W | ATTR::C)
      // section 15.3.4.2 Function.prototype.toString()
      .def<&runtime::FunctionToString, 0>(symbol::toString())
      // section 15.3.4.3 Function.prototype.apply(thisArg, argArray)
      .def<&runtime::FunctionApply, 2>("apply")
      // section 15.3.4.4 Function.prototype.call(thisArg[, arg1[, arg2, ...]])
      .def<&runtime::FunctionCall, 1>("call")
      // section 15.3.4.5 Function.prototype.bind(thisArg[, arg1[, arg2, ...]])
      .def<&runtime::FunctionBind, 1>("bind");

  bind::Object(this, obj_proto)
      .cls(obj_cls.cls)
      .prototype(NULL)
      // section 15.2.4.1 Object.prototype.constructor
      .def(symbol::constructor(), obj_constructor, ATTR::W | ATTR::C)
      // section 15.2.4.2 Object.prototype.toString()
      .def<&runtime::ObjectToString, 0>(symbol::toString())
      // section 15.2.4.3 Object.prototype.toLocaleString()
      .def<&runtime::ObjectToLocaleString, 0>("toLocaleString")
      // section 15.2.4.4 Object.prototype.valueOf()
      .def<&runtime::ObjectValueOf, 0>("valueOf")
      // section 15.2.4.5 Object.prototype.hasOwnProperty(V)
      .def<&runtime::ObjectHasOwnProperty, 1>("hasOwnProperty")
      // section 15.2.4.6 Object.prototype.isPrototypeOf(V)
      .def<&runtime::ObjectIsPrototypeOf, 1>("isPrototypeOf")
      // section 15.2.4.7 Object.prototype.propertyIsEnumerable(V)
      .def<&runtime::ObjectPropertyIsEnumerable, 1>("propertyIsEnumerable");

  global_binder.def(func_cls.name, func_constructor, ATTR::W | ATTR::C);
  global_binder.def(obj_cls.name, obj_constructor, ATTR::W | ATTR::C);
  throw_type_error_->Initialize(this);  // lazy update

  // section 15.1 Global
  InitGlobal(func_cls, obj_proto, eval_function, &global_binder);
  // section 15.4 Array
  InitArray(func_cls, obj_proto, &global_binder);
  // section 15.5 String
  InitString(func_cls, obj_proto, &global_binder);
  // section 15.6 Boolean
  InitBoolean(func_cls, obj_proto, &global_binder);
  // section 15.7 Number
  InitNumber(func_cls, obj_proto, &global_binder);
  // section 15.8 Math
  InitMath(func_cls, obj_proto, &global_binder);
  // section 15.9 Date
  InitDate(func_cls, obj_proto, &global_binder);
  // section 15.10 RegExp
  InitRegExp(func_cls, obj_proto, &global_binder);
  // section 15.11 Error
  InitError(func_cls, obj_proto, &global_binder);
  // section 15.12 JSON
  InitJSON(func_cls, obj_proto, &global_binder);

  // ES.next
  InitIntl(func_cls, obj_proto, &global_binder);
  InitMap(func_cls, obj_proto, &global_binder);
  InitSet(func_cls, obj_proto, &global_binder);

  {
    // Arguments
    struct ClassSlot cls = {
      JSArguments::GetClass(),
      context::Intern(this, "Arguments"),
      JSString::NewAsciiString(this, "Arguments"),
      NULL,
      obj_proto
    };
    global_data_.RegisterClass<Class::Arguments>(cls);
  }
}

void Context::InitGlobal(const ClassSlot& func_cls,
                         JSObject* obj_proto, JSFunction* eval_function,
                         bind::Object* global_binder) {
  // Global
  struct ClassSlot cls = {
    JSGlobal::GetClass(),
    symbol::global(),
    JSString::NewAsciiString(this, "global"),
    NULL,
    obj_proto
  };
  global_data_.RegisterClass<Class::global>(cls);
  eval_function->Initialize(this);  // lazy update
  global_binder->cls(cls.cls)
      .prototype(cls.prototype)
      // section 15.1.1.1 NaN
      .def("NaN", core::kNaN)
      // section 15.1.1.2 Infinity
      .def("Infinity", std::numeric_limits<double>::infinity())
      // section 15.1.1.3 undefined
      .def("undefined", JSUndefined)
      // section 15.1.2.1 eval(x)
      .def("eval", eval_function, ATTR::W | ATTR::C)
      // section 15.1.2.3 parseIng(string, radix)
      .def<&runtime::GlobalParseInt, 2>("parseInt")
      // section 15.1.2.3 parseFloat(string)
      .def<&runtime::GlobalParseFloat, 1>("parseFloat")
      // section 15.1.2.4 isNaN(number)
      .def<&runtime::GlobalIsNaN, 1>("isNaN")
      // section 15.1.2.5 isFinite(number)
      .def<&runtime::GlobalIsFinite, 1>("isFinite")
      // section 15.1.3.1 decodeURI(encodedURI)
      .def<&runtime::GlobalDecodeURI, 1>("decodeURI")
      // section 15.1.3.2 decodeURIComponent(encodedURIComponent)
      .def<&runtime::GlobalDecodeURIComponent, 1>("decodeURIComponent")
      // section 15.1.3.3 encodeURI(uri)
      .def<&runtime::GlobalEncodeURI, 1>("encodeURI")
      // section 15.1.3.4 encodeURIComponent(uriComponent)
      .def<&runtime::GlobalEncodeURIComponent, 1>("encodeURIComponent")
      // section B.2.1 escape(string)
      // this method is deprecated.
      .def<&runtime::GlobalEscape, 1>("escape")
      // section B.2.2 unescape(string)
      // this method is deprecated.
      .def<&runtime::GlobalUnescape, 1>("unescape");
}

void Context::InitArray(const ClassSlot& func_cls,
                        JSObject* obj_proto, bind::Object* global_binder) {
  // section 15.4 Array
  JSObject* const proto = JSArray::NewPlain(this, Map::NewUniqueMap(this));
  // section 15.4.2 The Array Constructor
  JSFunction* const constructor =
      JSInlinedFunction<&runtime::ArrayConstructor, 1>::NewPlain(
          this,
          context::Intern(this, "Array"));

  struct ClassSlot cls = {
    JSArray::GetClass(),
    context::Intern(this, "Array"),
    JSString::NewAsciiString(this, "Array"),
    constructor,
    proto
  };
  global_data_.RegisterClass<Class::Array>(cls);
  global_binder->def(cls.name, constructor, ATTR::W | ATTR::C);

  bind::Object(this, constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      // prototype
      .def(symbol::prototype(), proto, ATTR::NONE)
      // section 15.4.3.2 Array.isArray(arg)
      .def<&runtime::ArrayIsArray, 1>("isArray")
      // ES.next Array.from
      .def<&runtime::ArrayFrom, 1>("from")
      // ES.next Array.from
      .def<&runtime::ArrayOf, 0>("of");

  bind::Object(this, proto)
      .cls(cls.cls)
      .prototype(obj_proto)
      // section 15.5.4.1 Array.prototype.constructor
      .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
      // section 15.4.4.2 Array.prototype.toString()
      .def<&runtime::ArrayToString, 0>("toString")
      // section 15.4.4.3 Array.prototype.toLocaleString()
      .def<&runtime::ArrayToLocaleString, 0>("toLocaleString")
      // section 15.4.4.4 Array.prototype.concat([item1[, item2[, ...]]])
      .def<&runtime::ArrayConcat, 1>("concat")
      // section 15.4.4.5 Array.prototype.join(separator)
      .def<&runtime::ArrayJoin, 1>("join")
      // section 15.4.4.6 Array.prototype.pop()
      .def<&runtime::ArrayPop, 0>("pop")
      // section 15.4.4.7 Array.prototype.push([item1[, item2[, ...]]])
      .def<&runtime::ArrayPush, 1>("push")
      // section 15.4.4.8 Array.prototype.reverse()
      .def<&runtime::ArrayReverse, 0>("reverse")
      // section 15.4.4.9 Array.prototype.shift()
      .def<&runtime::ArrayShift, 0>("shift")
      // section 15.4.4.10 Array.prototype.slice(start, end)
      .def<&runtime::ArraySlice, 2>("slice")
      // section 15.4.4.11 Array.prototype.sort(comparefn)
      .def<&runtime::ArraySort, 1>("sort")
      // section 15.4.4.12
      // Array.prototype.splice(start, deleteCount[, item1[, item2[, ...]]])
      .def<&runtime::ArraySplice, 2>("splice")
      // section 15.4.4.13 Array.prototype.unshift([item1[, item2[, ...]]])
      .def<&runtime::ArrayUnshift, 1>("unshift")
      // section 15.4.4.14 Array.prototype.indexOf(searchElement[, fromIndex])
      .def<&runtime::ArrayIndexOf, 1>("indexOf")
      // section 15.4.4.15
      // Array.prototype.lastIndexOf(searchElement[, fromIndex])
      .def<&runtime::ArrayLastIndexOf, 1>("lastIndexOf")
      // section 15.4.4.16 Array.prototype.every(callbackfn[, thisArg])
      .def<&runtime::ArrayEvery, 1>("every")
      // section 15.4.4.17 Array.prototype.some(callbackfn[, thisArg])
      .def<&runtime::ArraySome, 1>("some")
      // section 15.4.4.18 Array.prototype.forEach(callbackfn[, thisArg])
      .def<&runtime::ArrayForEach, 1>("forEach")
      // section 15.4.4.19 Array.prototype.map(callbackfn[, thisArg])
      .def<&runtime::ArrayMap, 1>("map")
      // section 15.4.4.20 Array.prototype.filter(callbackfn[, thisArg])
      .def<&runtime::ArrayFilter, 1>("filter")
      // section 15.4.4.21 Array.prototype.reduce(callbackfn[, initialValue])
      .def<&runtime::ArrayReduce, 1>("reduce")
      // section 15.4.4.22
      // Array.prototype.reduceRight(callbackfn[, initialValue])
      .def<&runtime::ArrayReduceRight, 1>("reduceRight");
}

void Context::InitString(const ClassSlot& func_cls,
                         JSObject* obj_proto, bind::Object* global_binder) {
  // section 15.5 String
  JSStringObject* const proto = JSStringObject::NewPlain(this);
  // section 15.5.2 The String Constructor
  JSFunction* const constructor =
      JSInlinedFunction<&runtime::StringConstructor, 1>::NewPlain(
          this,
          context::Intern(this, "String"));

  struct ClassSlot cls = {
    JSStringObject::GetClass(),
    context::Intern(this, "String"),
    JSString::NewAsciiString(this, "String"),
    constructor,
    proto
  };
  global_data_.RegisterClass<Class::String>(cls);
  global_binder->def(cls.name, constructor, ATTR::W | ATTR::C);

  bind::Object(this, constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      // prototype
      .def(symbol::prototype(), proto, ATTR::NONE)
      // section 15.5.3.2 String.fromCharCode([char0 [, char1[, ...]]])
      .def<&runtime::StringFromCharCode, 1>("fromCharCode");

  bind::Object(this, proto)
      .cls(cls.cls)
      .prototype(obj_proto)
      // section 15.5.4.1 String.prototype.constructor
      .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
      // section 15.5.4.2 String.prototype.toString()
      .def<&runtime::StringToString, 0>("toString")
      // section 15.5.4.3 String.prototype.valueOf()
      .def<&runtime::StringValueOf, 0>("valueOf")
      // section 15.5.4.4 String.prototype.charAt(pos)
      .def<&runtime::StringCharAt, 1>("charAt")
      // section 15.5.4.5 String.prototype.charCodeAt(pos)
      .def<&runtime::StringCharCodeAt, 1>("charCodeAt")
      // section 15.5.4.6 String.prototype.concat([string1[, string2[, ...]]])
      .def<&runtime::StringConcat, 1>("concat")
      // section 15.5.4.7 String.prototype.indexOf(searchString, position)
      .def<&runtime::StringIndexOf, 1>("indexOf")
      // section 15.5.4.8 String.prototype.lastIndexOf(searchString, position)
      .def<&runtime::StringLastIndexOf, 1>("lastIndexOf")
      // section 15.5.4.9 String.prototype.localeCompare(that)
      .def<&runtime::StringLocaleCompare, 1>("localeCompare")
      // section 15.5.4.10 String.prototype.match(regexp)
      .def<&runtime::StringMatch, 1>("match")
      // section 15.5.4.11 String.prototype.replace(searchValue, replaceValue)
      .def<&runtime::StringReplace, 2>("replace")
      // section 15.5.4.12 String.prototype.search(regexp)
      .def<&runtime::StringSearch, 1>("search")
      // section 15.5.4.13 String.prototype.slice(start, end)
      .def<&runtime::StringSlice, 2>("slice")
      // section 15.5.4.14 String.prototype.split(separator, limit)
      .def<&runtime::StringSplit, 2>("split")
      // section 15.5.4.15 String.prototype.substring(start, end)
      .def<&runtime::StringSubstring, 2>("substring")
      // section 15.5.4.16 String.prototype.toLowerCase()
      .def<&runtime::StringToLowerCase, 0>("toLowerCase")
      // section 15.5.4.17 String.prototype.toLocaleLowerCase()
      .def<&runtime::StringToLocaleLowerCase, 0>("toLocaleLowerCase")
      // section 15.5.4.18 String.prototype.toUpperCase()
      .def<&runtime::StringToUpperCase, 0>("toUpperCase")
      // section 15.5.4.19 String.prototype.toLocaleUpperCase()
      .def<&runtime::StringToLocaleUpperCase, 0>("toLocaleUpperCase")
      // section 15.5.4.20 String.prototype.trim()
      .def<&runtime::StringTrim, 0>("trim")
      // section 15.5.4.21 String.prototype.repeat(count)
      .def<&runtime::StringRepeat, 1>("repeat")
      // section 15.5.4.22 String.prototype.startsWith(searchString, [position])
      .def<&runtime::StringStartsWith, 1>("startsWith")
      // section 15.5.4.23
      // String.prototype.endsWith(searchString, [endPosition])
      .def<&runtime::StringEndsWith, 1>("endsWith")
      // section 15.5.4.24 String.prototype.contains(searchString, [position])
      .def<&runtime::StringContains, 1>("contains")
      // section 15.5.4.25 String.prototype.toArray()
      .def<&runtime::StringToArray, 0>("toArray")
      // section 15.5.4.26 String.prototype.reverse()
      .def<&runtime::StringReverse, 0>("reverse")
      // section B.2.3 String.prototype.substr(start, length)
      // this method is deprecated.
      .def<&runtime::StringSubstr, 2>("substr");
}

void Context::InitBoolean(const ClassSlot& func_cls,
                          JSObject* obj_proto, bind::Object* global_binder) {
  // Boolean
  JSBooleanObject* const proto =
      JSBooleanObject::NewPlain(this, Map::NewUniqueMap(this), false);
  // section 15.6.2 The Boolean Constructor
  JSFunction* const constructor =
      JSInlinedFunction<&runtime::BooleanConstructor, 1>::NewPlain(
          this,
          context::Intern(this, "Boolean"));

  struct ClassSlot cls = {
    JSBooleanObject::GetClass(),
    context::Intern(this, "Boolean"),
    JSString::NewAsciiString(this, "Boolean"),
    constructor,
    proto
  };
  global_data_.RegisterClass<Class::Boolean>(cls);
  global_binder->def(cls.name, constructor, ATTR::W | ATTR::C);

  bind::Object(this, constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      // section 15.6.3.1 Boolean.prototype
      .def(symbol::prototype(), proto, ATTR::NONE);

  bind::Object(this, proto)
      .cls(cls.cls)
      .prototype(obj_proto)
      // section 15.6.4.1 Boolean.prototype.constructor
      .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
      // section 15.6.4.2 Boolean.prototype.toString()
      .def<&runtime::BooleanToString, 0>(symbol::toString())
      // section 15.6.4.3 Boolean.prototype.valueOf()
      .def<&runtime::BooleanValueOf, 0>(symbol::valueOf());
}

void Context::InitNumber(const ClassSlot& func_cls,
                         JSObject* obj_proto, bind::Object* global_binder) {
  // 15.7 Number
  JSNumberObject* const proto =
      JSNumberObject::NewPlain(this, Map::NewUniqueMap(this), 0);
  // section 15.7.3 The Number Constructor
  JSFunction* const constructor =
      JSInlinedFunction<&runtime::NumberConstructor, 1>::NewPlain(
          this,
          context::Intern(this, "Number"));

  struct ClassSlot cls = {
    JSNumberObject::GetClass(),
    context::Intern(this, "Number"),
    JSString::NewAsciiString(this, "Number"),
    constructor,
    proto
  };
  global_data_.RegisterClass<Class::Number>(cls);
  global_binder->def(cls.name, constructor, ATTR::W | ATTR::C);

  bind::Object(this, constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      // section 15.7.3.1 Number.prototype
      .def(symbol::prototype(), proto, ATTR::NONE)
      // section 15.7.3.2 Number.MAX_VALUE
      .def("MAX_VALUE", 1.7976931348623157e+308)
      // section 15.7.3.3 Number.MIN_VALUE
      .def("MIN_VALUE", 5e-324)
      // section 15.7.3.4 Number.NaN
      .def("NaN", JSNaN)
      // section 15.7.3.5 Number.NEGATIVE_INFINITY
      .def("NEGATIVE_INFINITY", -std::numeric_limits<double>::infinity())
      // section 15.7.3.6 Number.POSITIVE_INFINITY
      .def("POSITIVE_INFINITY", std::numeric_limits<double>::infinity())
      // section 15.7.3.7 Number.EPSILON
      .def("EPSILON", DBL_EPSILON)
      // section 15.7.3.8 Number.MAX_INTEGER
      .def("MAX_INTEGER", 9007199254740991.0)
      // section 15.7.3.9 Number.parseInt(string, radix)
      .def<&runtime::GlobalParseInt, 2>("parseInt")
      // section 15.7.3.10 Number.parseFloat(string)
      .def<&runtime::GlobalParseFloat, 1>("parseFloat")
      // section 15.7.3.11 Number.isNaN(number)
      .def<&runtime::NumberIsNaN, 1>("isNaN")
      // section 15.7.3.12 Number.isFinite(number)
      .def<&runtime::NumberIsFinite, 1>("isFinite")
      // section 15.7.3.13 Number.isInteger(number)
      .def<&runtime::NumberIsInteger, 1>("isInteger")
      // section 15.7.3.14 Number.toInt(number)
      .def<&runtime::NumberToInt, 1>("toInt");

  bind::Object(this, proto)
      .cls(cls.cls)
      .prototype(obj_proto)
      // section 15.7.4.1 Number.prototype.constructor
      .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
      // section 15.7.4.2 Number.prototype.toString([radix])
      .def<&runtime::NumberToString, 1>(symbol::toString())
      // section 15.7.4.3 Number.prototype.toLocaleString()
      .def<&runtime::NumberToLocaleString, 0>("toLocaleString")
      // section 15.7.4.4 Number.prototype.valueOf()
      .def<&runtime::NumberValueOf, 0>(symbol::valueOf())
      // section 15.7.4.5 Number.prototype.toFixed(fractionDigits)
      .def<&runtime::NumberToFixed, 1>("toFixed")
      // section 15.7.4.6 Number.prototype.toExponential(fractionDigits)
      .def<&runtime::NumberToExponential, 1>("toExponential")
      // section 15.7.4.7 Number.prototype.toPrecision(precision)
      .def<&runtime::NumberToPrecision, 1>("toPrecision")
      // section 15.7.4.8 Number.prototype.clz()
      .def<&runtime::NumberCLZ, 0>("clz");
}

void Context::InitMath(const ClassSlot& func_cls,
                       JSObject* obj_proto, bind::Object* global_binder) {
  // section 15.8 Math
  struct ClassSlot cls = {
    JSMath::GetClass(),
    context::Intern(this, "Math"),
    JSString::NewAsciiString(this, "Math"),
    NULL,
    obj_proto
  };
  global_data_.RegisterClass<Class::Math>(cls);
  JSObject* const math = JSMath::NewPlain(this);
  global_binder->def("Math", math, ATTR::W | ATTR::C);

  bind::Object(this, math)
      .cls(cls.cls)
      .prototype(obj_proto)
      // section 15.8.1.1 E
      .def("E", std::exp(1.0))
      // section 15.8.1.2 LN10
      .def("LN10", std::log(10.0))
      // section 15.8.1.3 LN2
      .def("LN2", std::log(2.0))
      // section 15.8.1.4 LOG2E
      .def("LOG2E", 1.0 / std::log(2.0))
      // section 15.8.1.5 LOG10E
      .def("LOG10E", 1.0 / std::log(10.0))
      // section 15.8.1.6 PI
      .def("PI", std::acos(-1.0))
      // section 15.8.1.7 SQRT1_2
      .def("SQRT1_2", std::sqrt(0.5))
      // section 15.8.1.8 SQRT2
      .def("SQRT2", std::sqrt(2.0))
      // section 15.8.2.1 abs(x)
      .def<&runtime::MathAbs, 1>("abs")
      // section 15.8.2.2 acos(x)
      .def<&runtime::MathAcos, 1>("acos")
      // section 15.8.2.3 asin(x)
      .def<&runtime::MathAsin, 1>("asin")
      // section 15.8.2.4 atan(x)
      .def<&runtime::MathAtan, 1>("atan")
      // section 15.8.2.5 atan2(y, x)
      .def<&runtime::MathAtan2, 2>("atan2")
      // section 15.8.2.6 ceil(x)
      .def<&runtime::MathCeil, 1>("ceil")
      // section 15.8.2.7 cos(x)
      .def<&runtime::MathCos, 1>("cos")
      // section 15.8.2.8 exp(x)
      .def<&runtime::MathExp, 1>("exp")
      // section 15.8.2.9 floor(x)
      .def<&runtime::MathFloor, 1>("floor")
      // section 15.8.2.10 log(x)
      .def<&runtime::MathLog, 1>("log")
      // section 15.8.2.11 max([value1[, value2[, ... ]]])
      .def<&runtime::MathMax, 2>("max")
      // section 15.8.2.12 min([value1[, value2[, ... ]]])
      .def<&runtime::MathMin, 2>("min")
      // section 15.8.2.13 pow(x, y)
      .def<&runtime::MathPow, 2>("pow")
      // section 15.8.2.14 random()
      .def<&runtime::MathRandom, 0>("random")
      // section 15.8.2.15 round(x)
      .def<&runtime::MathRound, 1>("round")
      // section 15.8.2.16 sin(x)
      .def<&runtime::MathSin, 1>("sin")
      // section 15.8.2.17 sqrt(x)
      .def<&runtime::MathSqrt, 1>("sqrt")
      // section 15.8.2.18 tan(x)
      .def<&runtime::MathTan, 1>("tan")
      // section 15.8.2.19 log10(x)
      .def<&runtime::MathLog10, 1>("log10")
      // section 15.8.2.20 log2(x)
      .def<&runtime::MathLog2, 1>("log2")
      // section 15.8.2.21 log1p(x)
      .def<&runtime::MathLog1p, 1>("log1p")
      // section 15.8.2.22 expm1(x)
      .def<&runtime::MathExpm1, 1>("expm1")
      // section 15.8.2.23 cosh(x)
      .def<&runtime::MathCosh, 1>("cosh")
      // section 15.8.2.24 sinh(x)
      .def<&runtime::MathSinh, 1>("sinh")
      // section 15.8.2.25 tanh(x)
      .def<&runtime::MathTanh, 1>("tanh")
      // section 15.8.2.26 acosh(x)
      .def<&runtime::MathAcosh, 1>("acosh")
      // section 15.8.2.27 asinh(x)
      .def<&runtime::MathAsinh, 1>("asinh")
      // section 15.8.2.28 atanh(x)
      .def<&runtime::MathAtanh, 1>("atanh")
      // section 15.8.2.29 hypot(value1, value2[, value3])
      .def<&runtime::MathHypot, 2>("hypot")
      // section 15.8.2.30 trunc(x)
      .def<&runtime::MathTrunc, 1>("trunc")
      // section 15.8.2.31 sign(x)
      .def<&runtime::MathSign, 1>("sign")
      // section 15.8.2.32 cbrt(x)
      .def<&runtime::MathCbrt, 1>("cbrt");
}

void Context::InitDate(const ClassSlot& func_cls,
                       JSObject* obj_proto, bind::Object* global_binder) {
  // section 15.9 Date
  JSObject* const proto =
      JSDate::NewPlain(this, Map::NewUniqueMap(this), core::kNaN);
  // section 15.9.2.1 The Date Constructor
  JSFunction* const constructor =
      JSInlinedFunction<&runtime::DateConstructor, 7>::NewPlain(
          this,
          context::Intern(this, "Date"));

  struct ClassSlot cls = {
    JSDate::GetClass(),
    context::Intern(this, "Date"),
    JSString::NewAsciiString(this, "Date"),
    constructor,
    proto
  };
  global_data_.RegisterClass<Class::Date>(cls);
  global_binder->def(cls.name, constructor, ATTR::W | ATTR::C);

  bind::Object(this, constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      // section 15.9.4.1 Date.prototype
      .def(symbol::prototype(), proto, ATTR::NONE)
      // section 15.9.4.2 Date.parse(string)
      .def<&runtime::DateParse, 1>("parse")
      // section 15.9.4.3 Date.UTC()
      .def<&runtime::DateUTC, 7>("UTC")
      // section 15.9.4.4 Date.now()
      .def<&runtime::DateNow, 0>("now");

  JSFunction* const toUTCString =
      JSInlinedFunction<&runtime::DateToUTCString, 0>::New(
          this, context::Intern(this, "toUTCString"));

  bind::Object(this, proto)
      .cls(cls.cls)
      .prototype(obj_proto)
      // section 15.9.5.1 Date.prototype.constructor
      .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
      // section 15.9.5.2 Date.prototype.toString()
      .def<&runtime::DateToString, 0>("toString")
      // section 15.9.5.3 Date.prototype.toDateString()
      .def<&runtime::DateToDateString, 0>("toDateString")
      // section 15.9.5.4 Date.prototype.toTimeString()
      .def<&runtime::DateToTimeString, 0>("toTimeString")
      // section 15.9.5.5 Date.prototype.toLocaleString()
      .def<&runtime::DateToLocaleString, 0>("toLocaleString")
      // section 15.9.5.6 Date.prototype.toLocaleDateString()
      .def<&runtime::DateToLocaleDateString, 0>("toLocaleDateString")
      // section 15.9.5.7 Date.prototype.toLocaleTimeString()
      .def<&runtime::DateToLocaleTimeString, 0>("toLocaleTimeString")
      // section 15.9.5.8 Date.prototype.valueOf()
      .def<&runtime::DateValueOf, 0>("valueOf")
      // section 15.9.5.9 Date.prototype.getTime()
      .def<&runtime::DateGetTime, 0>("getTime")
      // section 15.9.5.10 Date.prototype.getFullYear()
      .def<&runtime::DateGetFullYear, 0>("getFullYear")
      // section 15.9.5.11 Date.prototype.getUTCFullYear()
      .def<&runtime::DateGetUTCFullYear, 0>("getUTCFullYear")
      // section 15.9.5.12 Date.prototype.getMonth()
      .def<&runtime::DateGetMonth, 0>("getMonth")
      // section 15.9.5.13 Date.prototype.getUTCMonth()
      .def<&runtime::DateGetUTCMonth, 0>("getUTCMonth")
      // section 15.9.5.14 Date.prototype.getDate()
      .def<&runtime::DateGetDate, 0>("getDate")
      // section 15.9.5.15 Date.prototype.getUTCDate()
      .def<&runtime::DateGetUTCDate, 0>("getUTCDate")
      // section 15.9.5.16 Date.prototype.getDay()
      .def<&runtime::DateGetDay, 0>("getDay")
      // section 15.9.5.17 Date.prototype.getUTCDay()
      .def<&runtime::DateGetUTCDay, 0>("getUTCDay")
      // section 15.9.5.18 Date.prototype.getHours()
      .def<&runtime::DateGetHours, 0>("getHours")
      // section 15.9.5.19 Date.prototype.getUTCHours()
      .def<&runtime::DateGetUTCHours, 0>("getUTCHours")
      // section 15.9.5.20 Date.prototype.getMinutes()
      .def<&runtime::DateGetMinutes, 0>("getMinutes")
      // section 15.9.5.21 Date.prototype.getUTCMinutes()
      .def<&runtime::DateGetUTCMinutes, 0>("getUTCMinutes")
      // section 15.9.5.22 Date.prototype.getSeconds()
      .def<&runtime::DateGetSeconds, 0>("getSeconds")
      // section 15.9.5.23 Date.prototype.getUTCSeconds()
      .def<&runtime::DateGetUTCSeconds, 0>("getUTCSeconds")
      // section 15.9.5.24 Date.prototype.getMilliseconds()
      .def<&runtime::DateGetMilliseconds, 0>("getMilliseconds")
      // section 15.9.5.25 Date.prototype.getUTCMilliseconds()
      .def<&runtime::DateGetUTCMilliseconds, 0>("getUTCMilliseconds")
      // section 15.9.5.26 Date.prototype.getTimezoneOffset()
      .def<&runtime::DateGetTimezoneOffset, 0>("getTimezoneOffset")
      // section 15.9.5.27 Date.prototype.setTime(time)
      .def<&runtime::DateSetTime, 1>("setTime")
      // section 15.9.5.28 Date.prototype.setMilliseconds(ms)
      .def<&runtime::DateSetMilliseconds, 1>("setMilliseconds")
      // section 15.9.5.29 Date.prototype.setUTCMilliseconds(ms)
      .def<&runtime::DateSetUTCMilliseconds, 1>("setUTCMilliseconds")
      // section 15.9.5.30 Date.prototype.setSeconds(sec[, ms])
      .def<&runtime::DateSetSeconds, 2>("setSeconds")
      // section 15.9.5.31 Date.prototype.setUTCSeconds(sec[, ms])
      .def<&runtime::DateSetUTCSeconds, 2>("setUTCSeconds")
      // section 15.9.5.32 Date.prototype.setMinutes(min[, sec[, ms]])
      .def<&runtime::DateSetMinutes, 3>("setMinutes")
      // section 15.9.5.33 Date.prototype.setUTCMinutes(min[, sec[, ms]])
      .def<&runtime::DateSetUTCMinutes, 3>("setUTCMinutes")
      // section 15.9.5.34 Date.prototype.setHours(hour[, min[, sec[, ms]])
      .def<&runtime::DateSetHours, 4>("setHours")
      // section 15.9.5.35 Date.prototype.setUTCHours(hour[, min[, sec[, ms]])
      .def<&runtime::DateSetUTCHours, 4>("setUTCHours")
      // section 15.9.5.36 Date.prototype.setDate(date)
      .def<&runtime::DateSetDate, 1>("setDate")
      // section 15.9.5.37 Date.prototype.setUTCDate(date)
      .def<&runtime::DateSetUTCDate, 1>("setUTCDate")
      // section 15.9.5.38 Date.prototype.setMonth(month[, date])
      .def<&runtime::DateSetMonth, 2>("setMonth")
      // section 15.9.5.39 Date.prototype.setUTCMonth(month[, date])
      .def<&runtime::DateSetUTCMonth, 2>("setUTCMonth")
      // section 15.9.5.40 Date.prototype.setFullYear(year[, month[, date]])
      .def<&runtime::DateSetFullYear, 3>("setFullYear")
      // section 15.9.5.41 Date.prototype.setUTCFullYear(year[, month[, date]])
      .def<&runtime::DateSetUTCFullYear, 3>("setUTCFullYear")
      // section 15.9.5.42 Date.prototype.toUTCString()
      .def("toUTCString", toUTCString, ATTR::W | ATTR::C)
      // section 15.9.5.43 Date.prototype.toISOString()
      .def<&runtime::DateToISOString, 0>("toISOString")
      // section 15.9.5.44 Date.prototype.toJSON()
      .def<&runtime::DateToJSON, 1>("toJSON")
      // section B.2.4 Date.prototype.getYear()
      // this method is deprecated.
      .def<&runtime::DateGetYear, 0>("getYear")
      // section B.2.5 Date.prototype.setYear(year)
      // this method is deprecated.
      .def<&runtime::DateSetYear, 1>("setYear")
      // section B.2.6 Date.prototype.toGMTString()
      .def("toGMTString", toUTCString, ATTR::W | ATTR::C);
}

void Context::InitRegExp(const ClassSlot& func_cls,
                         JSObject* obj_proto, bind::Object* global_binder) {
  // section 15.10 RegExp
  JSObject* const proto = JSRegExp::NewPlain(this, Map::NewUniqueMap(this));
  // section 15.10.4 The RegExp Constructor
  JSFunction* const constructor =
      JSInlinedFunction<&runtime::RegExpConstructor, 2>::NewPlain(
          this,
          context::Intern(this, "RegExp"));

  struct ClassSlot cls = {
    JSRegExp::GetClass(),
    context::Intern(this, "RegExp"),
    JSString::NewAsciiString(this, "RegExp"),
    constructor,
    proto
  };
  global_data_.RegisterClass<Class::RegExp>(cls);
  global_binder->def(cls.name, constructor, ATTR::W | ATTR::C);

  bind::Object(this, constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      // section 15.10.5.1 RegExp.prototype
      .def(symbol::prototype(), proto, ATTR::NONE);

  bind::Object(this, proto)
      .cls(cls.cls)
      .prototype(obj_proto)
      // section 15.10.6.1 RegExp.prototype.constructor
      .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
      // section 15.10.6.2 RegExp.prototype.exec(string)
      .def<&runtime::RegExpExec, 1>("exec")
      // section 15.10.6.3 RegExp.prototype.test(string)
      .def<&runtime::RegExpTest, 1>("test")
      // section 15.10.6.4 RegExp.prototype.toString()
      .def<&runtime::RegExpToString, 0>("toString")
      // Not Standard RegExp.prototype.compile(pattern, flags)
      .def<&runtime::RegExpCompile, 2>("compile");
}

void Context::InitError(const ClassSlot& func_cls,
                        JSObject* obj_proto, bind::Object* global_binder) {
  // Error
  JSObject* const proto = JSObject::NewPlain(this, Map::NewUniqueMap(this));
  // section 15.11.2 The Error Constructor
  JSFunction* const constructor =
      JSInlinedFunction<&runtime::ErrorConstructor, 1>::NewPlain(
          this,
          context::Intern(this, "Error"));

  struct ClassSlot cls = {
    JSError::GetClass(),
    context::Intern(this, "Error"),
    JSString::NewAsciiString(this, "Error"),
    constructor,
    proto
  };
  global_data_.RegisterClass<Class::Error>(cls);
  global_binder->def(cls.name, constructor, ATTR::W | ATTR::C);

  bind::Object(this, constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      // section 15.11.3.1 Error.prototype
      .def(symbol::prototype(), proto, ATTR::NONE);


  bind::Object(this, proto)
      .cls(cls.cls)
      .prototype(obj_proto)
      // section 15.11.4.1 Error.prototype.constructor
      .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
      // section 15.11.4.2 Error.prototype.name
      .def("name", JSString::NewAsciiString(this, "Error"), ATTR::W | ATTR::C)
      // section 15.11.4.3 Error.prototype.message
      .def("message", JSString::NewEmptyString(this), ATTR::W | ATTR::C)
      // section 15.11.4.4 Error.prototype.toString()
      .def<&runtime::ErrorToString, 0>(symbol::toString());

  {
    // section 15.11.6.1 EvalError
    JSObject* const sub_proto =
        JSObject::NewPlain(this, Map::NewUniqueMap(this));
    const Symbol sym = context::Intern(this, "EvalError");
    JSFunction* const sub_constructor =
        JSInlinedFunction<&runtime::EvalErrorConstructor, 1>::NewPlain(
            this,
            sym);
    bind::Object(this, sub_constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), sub_proto, ATTR::NONE);

    struct ClassSlot sub_cls = {
      JSEvalError::GetClass(),
      context::Intern(this, "Error"),
      JSString::NewAsciiString(this, "EvalError"),
      sub_constructor,
      sub_proto
    };
    global_data_.RegisterClass<Class::EvalError>(sub_cls);
    global_binder->def(sym, sub_constructor, ATTR::W | ATTR::C);

    bind::Object(this, sub_proto)
        .cls(sub_cls.cls)
        .prototype(proto)
        .def(symbol::constructor(),
             sub_constructor, ATTR::W | ATTR::C)
        .def("name", sub_cls.name_string, ATTR::W | ATTR::C)
        .def("message", JSString::NewEmptyString(this), ATTR::W | ATTR::C);
  }
  {
    // section 15.11.6.2 RangeError
    JSObject* const sub_proto =
        JSObject::NewPlain(this, Map::NewUniqueMap(this));
    const Symbol sym = context::Intern(this, "RangeError");
    JSFunction* const sub_constructor =
        JSInlinedFunction<&runtime::RangeErrorConstructor, 1>::NewPlain(
            this,
            sym);
    bind::Object(this, sub_constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), sub_proto, ATTR::NONE);

    struct ClassSlot sub_cls = {
      JSRangeError::GetClass(),
      context::Intern(this, "Error"),
      JSString::NewAsciiString(this, "RangeError"),
      sub_constructor,
      sub_proto
    };
    global_data_.RegisterClass<Class::RangeError>(sub_cls);
    global_binder->def(sym, sub_constructor, ATTR::W | ATTR::C);

    bind::Object(this, sub_proto)
        .cls(sub_cls.cls)
        .prototype(proto)
        .def(symbol::constructor(),
             sub_constructor, ATTR::W | ATTR::C)
        .def("name", sub_cls.name_string, ATTR::W | ATTR::C)
        .def("message", JSString::NewEmptyString(this), ATTR::W | ATTR::C);
  }
  {
    // section 15.11.6.3 ReferenceError
    JSObject* const sub_proto =
        JSObject::NewPlain(this, Map::NewUniqueMap(this));
    const Symbol sym = context::Intern(this, "ReferenceError");
    JSFunction* const sub_constructor =
        JSInlinedFunction<
          &runtime::ReferenceErrorConstructor, 1>::NewPlain(
            this,
            sym);
    bind::Object(this, sub_constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), sub_proto, ATTR::NONE);

    struct ClassSlot sub_cls = {
      JSReferenceError::GetClass(),
      context::Intern(this, "Error"),
      JSString::NewAsciiString(this, "ReferenceError"),
      sub_constructor,
      sub_proto
    };
    global_data_.RegisterClass<Class::ReferenceError>(sub_cls);
    global_binder->def(sym, sub_constructor, ATTR::W | ATTR::C);

    bind::Object(this, sub_proto)
        .cls(sub_cls.cls)
        .prototype(proto)
        .def(symbol::constructor(),
             sub_constructor, ATTR::W | ATTR::C)
        .def("name", sub_cls.name_string, ATTR::W | ATTR::C)
        .def("message", JSString::NewEmptyString(this), ATTR::W | ATTR::C);
  }
  {
    // section 15.11.6.4 SyntaxError
    JSObject* const sub_proto =
        JSObject::NewPlain(this, Map::NewUniqueMap(this));
    const Symbol sym = context::Intern(this, "SyntaxError");
    JSFunction* const sub_constructor =
        JSInlinedFunction<&runtime::SyntaxErrorConstructor, 1>::NewPlain(
            this,
            sym);
    bind::Object(this, sub_constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), sub_proto, ATTR::NONE);

    struct ClassSlot sub_cls = {
      JSSyntaxError::GetClass(),
      context::Intern(this, "Error"),
      JSString::NewAsciiString(this, "SyntaxError"),
      sub_constructor,
      sub_proto
    };
    global_data_.RegisterClass<Class::SyntaxError>(sub_cls);
    global_binder->def(sym, sub_constructor, ATTR::W | ATTR::C);

    bind::Object(this, sub_proto)
        .cls(sub_cls.cls)
        .prototype(proto)
        .def(symbol::constructor(),
             sub_constructor, ATTR::W | ATTR::C)
        .def("name", sub_cls.name_string, ATTR::W | ATTR::C)
        .def("message", JSString::NewEmptyString(this), ATTR::W | ATTR::C);
  }
  {
    // section 15.11.6.5 TypeError
    JSObject* const sub_proto =
        JSObject::NewPlain(this, Map::NewUniqueMap(this));
    const Symbol sym = context::Intern(this, "TypeError");
    JSFunction* const sub_constructor =
        JSInlinedFunction<&runtime::TypeErrorConstructor, 1>::NewPlain(
            this,
            sym);
    bind::Object(this, sub_constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), sub_proto, ATTR::NONE);

    struct ClassSlot sub_cls = {
      JSTypeError::GetClass(),
      context::Intern(this, "Error"),
      JSString::NewAsciiString(this, "TypeError"),
      sub_constructor,
      sub_proto
    };
    global_data_.RegisterClass<Class::TypeError>(sub_cls);
    global_binder->def(sym, sub_constructor, ATTR::W | ATTR::C);

    bind::Object(this, sub_proto)
        .cls(sub_cls.cls)
        .prototype(proto)
        .def(symbol::constructor(),
             sub_constructor, ATTR::W | ATTR::C)
        .def("name", sub_cls.name_string, ATTR::W | ATTR::C)
        .def("message", JSString::NewEmptyString(this), ATTR::W | ATTR::C);
  }
  {
    // section 15.11.6.6 URIError
    JSObject* const sub_proto =
        JSObject::NewPlain(this, Map::NewUniqueMap(this));
    const Symbol sym = context::Intern(this, "URIError");
    JSFunction* const sub_constructor =
        JSInlinedFunction<&runtime::URIErrorConstructor, 1>::NewPlain(
            this,
            sym);
    bind::Object(this, sub_constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), sub_proto, ATTR::NONE);

    struct ClassSlot sub_cls = {
      JSURIError::GetClass(),
      context::Intern(this, "Error"),
      JSString::NewAsciiString(this, "URIError"),
      sub_constructor,
      sub_proto
    };
    global_data_.RegisterClass<Class::URIError>(sub_cls);
    global_binder->def(sym, sub_constructor, ATTR::W | ATTR::C);

    bind::Object(this, sub_proto)
        .cls(sub_cls.cls)
        .prototype(proto)
        .def(symbol::constructor(),
             sub_constructor, ATTR::W | ATTR::C)
        .def("name", sub_cls.name_string, ATTR::W | ATTR::C)
        .def("message", JSString::NewEmptyString(this), ATTR::W | ATTR::C);
  }
}

void Context::InitJSON(const ClassSlot& func_cls,
                       JSObject* obj_proto, bind::Object* global_binder) {
  // section 15.12 JSON
  struct ClassSlot cls = {
    JSJSON::GetClass(),
    context::Intern(this, "JSON"),
    JSString::NewAsciiString(this, "JSON"),
    NULL,
    obj_proto
  };
  global_data_.RegisterClass<Class::JSON>(cls);
  JSObject* const json = JSJSON::NewPlain(this, Map::NewUniqueMap(this));
  global_binder->def("JSON", json, ATTR::W | ATTR::C);
  bind::Object(this, json)
      .cls(cls.cls)
      .prototype(obj_proto)
      // section 15.12.2 parse(text[, reviver])
      .def<&runtime::JSONParse, 2>("parse")
      // section 15.12.3 stringify(value[, replacer[, space]])
      .def<&runtime::JSONStringify, 3>("stringify");
}

void Context::InitMap(const ClassSlot& func_cls,
                      JSObject* obj_proto, bind::Object* global_binder) {
  // ES.next Map
  // http://wiki.ecmascript.org/doku.php?id=harmony:simple_maps_and_sets
  JSObject* const proto = JSMap::NewPlain(this, Map::NewUniqueMap(this));
  JSFunction* const constructor =
      JSInlinedFunction<&runtime::MapConstructor, 0>::NewPlain(
          this,
          context::Intern(this, "Map"));

  struct ClassSlot cls = {
    JSMap::GetClass(),
    context::Intern(this, "Map"),
    JSString::NewAsciiString(this, "Map"),
    constructor,
    proto
  };
  global_data_.RegisterClass<Class::Map>(cls);
  global_binder->def(cls.name, constructor, ATTR::W | ATTR::C);

  bind::Object(this, constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      .def(symbol::prototype(), proto, ATTR::NONE);

  bind::Object(this, proto)
      .cls(cls.cls)
      .prototype(obj_proto)
      .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
      .def<&runtime::MapHas, 1>("has")
      .def<&runtime::MapGet, 1>("get")
      .def<&runtime::MapSet, 2>("set")
      .def<&runtime::MapDelete, 1>("delete");
}

void Context::InitSet(const ClassSlot& func_cls,
                      JSObject* obj_proto, bind::Object* global_binder) {
  // ES.next Set
  // http://wiki.ecmascript.org/doku.php?id=harmony:simple_maps_and_sets
  JSObject* const proto = JSSet::NewPlain(this, Map::NewUniqueMap(this));
  JSFunction* const constructor =
      JSInlinedFunction<&runtime::SetConstructor, 0>::NewPlain(
          this,
          context::Intern(this, "Set"));

  struct ClassSlot cls = {
    JSSet::GetClass(),
    context::Intern(this, "Set"),
    JSString::NewAsciiString(this, "Set"),
    constructor,
    proto
  };
  global_data_.RegisterClass<Class::Set>(cls);
  global_binder->def(cls.name, constructor, ATTR::W | ATTR::C);

  bind::Object(this, constructor)
      .cls(func_cls.cls)
      .prototype(func_cls.prototype)
      .def(symbol::prototype(), proto, ATTR::NONE);

  bind::Object(this, proto)
      .cls(cls.cls)
      .prototype(obj_proto)
      .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
      .def<&runtime::SetHas, 1>("has")
      .def<&runtime::SetAdd, 1>("add")
      .def<&runtime::SetDelete, 1>("delete");
}

void Context::InitIntl(const ClassSlot& func_cls,
                       JSObject* obj_proto, bind::Object* global_binder) {
  struct ClassSlot cls = {
    JSIntl::GetClass(),
    context::Intern(this, "Intl"),
    JSString::NewAsciiString(this, "Intl"),
    NULL,
    obj_proto
  };
  global_data_.RegisterClass<Class::Intl>(cls);
  JSObject* const intl = JSIntl::NewPlain(this);
  global_binder->def("Intl", intl, ATTR::W | ATTR::C);

  bind::Object intl_binder =
      bind::Object(this, intl)
        .cls(cls.cls)
        .prototype(obj_proto);

  {
    // LocaleList
    JSObject* const proto =
        JSLocaleList::NewPlain(this, Map::NewUniqueMap(this));
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::LocaleListConstructor, 1>::NewPlain(
            this,
            context::Intern(this, "LocaleList"));

    struct ClassSlot cls = {
      JSLocaleList::GetClass(),
      context::Intern(this, "LocaleList"),
      JSString::NewAsciiString(this, "LocaleList"),
      constructor,
      proto
    };
    global_data_.RegisterClass<Class::LocaleList>(cls);
    intl_binder.def(cls.name, constructor, ATTR::W | ATTR::C);

    bind::Object(this, constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), proto, ATTR::NONE);

    bind::Object(this, proto)
        .cls(cls.cls)
        .prototype(obj_proto)
        .def(symbol::constructor(), constructor, ATTR::W | ATTR::C);
  }

#ifdef IV_ENABLE_I18N
  // Currently, Collator, NumberFormat, DateTimeFormat need ICU
  {
    // Collator
    JSObject* const proto =
        JSCollator::NewPlain(this, Map::NewUniqueMap(this));
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::CollatorConstructor, 2>::NewPlain(
            this,
            context::Intern(this, "Collator"));

    struct ClassSlot cls = {
      JSCollator::GetClass(),
      context::Intern(this, "Collator"),
      JSString::NewAsciiString(this, "Collator"),
      constructor,
      proto
    };
    global_data_.RegisterClass<Class::Collator>(cls);
    intl_binder.def(cls.name, constructor, ATTR::W | ATTR::C);

    bind::Object(this, constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), proto, ATTR::NONE)
        .def<&runtime::CollatorSupportedLocalesOf, 1>("supportedLocalesOf");

    bind::Object(this, proto)
        .cls(cls.cls)
        .prototype(obj_proto)
        .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
        .def_getter<&runtime::CollatorCompareGetter, 0>(symbol::compare())
        .def_getter<
          &runtime::CollatorResolvedOptionsGetter, 0>("resolvedOptions");
  }

  {
    // NumberFormat
    JSObject* const proto =
        JSNumberFormat::NewPlain(this, Map::NewUniqueMap(this));
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::NumberFormatConstructor, 2>::NewPlain(
            this,
            context::Intern(this, "NumberFormat"));

    struct ClassSlot cls = {
      JSNumberFormat::GetClass(),
      context::Intern(this, "NumberFormat"),
      JSString::NewAsciiString(this, "NumberFormat"),
      constructor,
      proto
    };
    global_data_.RegisterClass<Class::NumberFormat>(cls);
    intl_binder.def(cls.name, constructor, ATTR::W | ATTR::C);

    bind::Object(this, constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), proto, ATTR::NONE)
        .def<&runtime::CollatorSupportedLocalesOf, 1>("supportedLocalesOf");

    bind::Object(this, proto)
        .cls(cls.cls)
        .prototype(obj_proto)
        .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
        .def<&runtime::NumberFormatFormat, 1>("format")
        .def_getter<
          &runtime::NumberFormatResolvedOptionsGetter, 0>("resolvedOptions");
  }

  {
    // DateTimeFormat
    JSObject* const proto =
        JSDateTimeFormat::NewPlain(this, Map::NewUniqueMap(this));
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::DateTimeFormatConstructor, 2>::NewPlain(
            this,
            context::Intern(this, "DateTimeFormat"));

    struct ClassSlot cls = {
      JSDateTimeFormat::GetClass(),
      context::Intern(this, "DateTimeFormat"),
      JSString::NewAsciiString(this, "DateTimeFormat"),
      constructor,
      proto
    };
    global_data_.RegisterClass<Class::DateTimeFormat>(cls);
    intl_binder.def(cls.name, constructor, ATTR::W | ATTR::C);

    bind::Object(this, constructor)
        .cls(func_cls.cls)
        .prototype(func_cls.prototype)
        .def(symbol::prototype(), proto, ATTR::NONE)
        .def<&runtime::CollatorSupportedLocalesOf, 1>("supportedLocalesOf");

    bind::Object(this, proto)
        .cls(cls.cls)
        .prototype(obj_proto)
        .def(symbol::constructor(), constructor, ATTR::W | ATTR::C)
        .def<&runtime::DateTimeFormatFormat, 1>("format")
        .def_getter<
          &runtime::DateTimeFormatResolvedOptionsGetter, 0>("resolvedOptions");
  }
#endif  // IV_ENABLE_I18N
}

} }  // namespace iv::lv5
