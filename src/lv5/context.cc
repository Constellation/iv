#include <cmath>
#include <cstdio>
#include <tr1/array>
#include <cstring>
#include "ustring.h"
#include "dtoa.h"
#include "lv5/jsfunction.h"
#include "lv5/arguments.h"
#include "lv5/context.h"
#include "lv5/context_utils.h"
#include "lv5/interpreter.h"
#include "lv5/property.h"
#include "lv5/class.h"
#include "lv5/runtime.h"
#include "lv5/jsast.h"
#include "lv5/jsscript.h"
#include "lv5/jserror.h"
#include "lv5/bind.h"
namespace iv {
namespace lv5 {
namespace {

static const std::string length_string("length");
static const std::string eval_string("eval");
static const std::string arguments_string("arguments");
static const std::string caller_string("caller");
static const std::string callee_string("callee");
static const std::string toString_string("toString");
static const std::string valueOf_string("valueOf");
static const std::string prototype_string("prototype");
static const std::string constructor_string("constructor");
static const std::string Array_string("Array");

class ScriptScope : private core::Noncopyable<ScriptScope>::type {
 public:
  ScriptScope(Context* ctx, JSScript* script)
    : ctx_(ctx),
      prev_(ctx->current_script()) {
    ctx_->set_current_script(script);
  }
  ~ScriptScope() {
    ctx_->set_current_script(prev_);
  }
 private:
  Context* ctx_;
  JSScript* prev_;
};

}  // namespace

namespace context {

const core::UString& GetSymbolString(const Context* ctx, const Symbol& sym) {
  return ctx->GetSymbolString(sym);
}

const Class& Cls(Context* ctx, const Symbol& name) {
  return ctx->Cls(name);
}

const Class& Cls(Context* ctx, const core::StringPiece& str) {
  return ctx->Cls(ctx->Intern(str));
}

Symbol Intern(Context* ctx, const core::StringPiece& str) {
  return ctx->Intern(str);
}

Symbol Intern(Context* ctx, const core::UStringPiece& str) {
  return ctx->Intern(str);
}

Symbol Intern(Context* ctx, const Identifier& ident) {
  return ctx->Intern(ident);
}

Symbol Intern(Context* ctx, uint32_t index) {
  return ctx->InternIndex(index);
}

Symbol Intern(Context* ctx, double number) {
  return ctx->InternDouble(number);
}

Symbol Lookup(Context* ctx, const core::StringPiece& str, bool* res) {
  return ctx->CheckIntern(str, res);
}

Symbol Lookup(Context* ctx, const core::UStringPiece& str, bool* res) {
  return ctx->CheckIntern(str, res);
}

Symbol Lookup(Context* ctx, uint32_t index, bool* res) {
  return ctx->CheckIntern(index, res);
}

Symbol Lookup(Context* ctx, double number, bool* res) {
  return ctx->CheckIntern(number, res);
}

Error* error(Context* ctx) {
  return ctx->error();
}

Symbol constructor_symbol(const Context* ctx) {
  return ctx->constructor_symbol();
}

Symbol caller_symbol(const Context* ctx) {
  return ctx->caller_symbol();
}

Symbol prototype_symbol(const Context* ctx) {
  return ctx->prototype_symbol();
}

Symbol length_symbol(const Context* ctx) {
  return ctx->length_symbol();
}

bool IsStrict(const Context* ctx) {
  return ctx->IsStrict();
}

VMStack* stack(Context* ctx) {
  return ctx->stack();
}

void RegisterLiteralRegExp(Context* ctx, JSRegExpImpl* reg) {
  ctx->regs().push_back(reg);
}

}  // namespace iv::lv5::context

Context::Context()
  : stack_resource_(),
    global_obj_(),
    lexical_env_(NULL),
    variable_env_(NULL),
    global_env_(NULL),
    binding_(&global_obj_),
    table_(),
    interp_(),
    mode_(NORMAL),
    ret_(),
    target_(NULL),
    error_(),
    regs_(),
    builtins_(),
    strict_(false),
    generate_script_counter_(0),
    random_engine_(random_engine_type(),
                   random_distribution_type(0, 1)),
    length_symbol_(Intern(length_string)),
    eval_symbol_(Intern(eval_string)),
    arguments_symbol_(Intern(arguments_string)),
    caller_symbol_(Intern(caller_string)),
    callee_symbol_(Intern(callee_string)),
    toString_symbol_(Intern(toString_string)),
    valueOf_symbol_(Intern(valueOf_string)),
    prototype_symbol_(Intern(prototype_string)),
    constructor_symbol_(Intern(constructor_string)),
    Array_symbol_(Intern(Array_string)),
    current_script_(NULL),
    throw_type_error_(this) {
  JSObjectEnv* const env = Interpreter::NewObjectEnvironment(this,
                                                             &global_obj_,
                                                             NULL);
  lexical_env_ = env;
  variable_env_ = env;
  global_env_ = env;
  env->set_provide_this(true);
  interp_.set_context(this);
  // discard random
  for (std::size_t i = 0; i < 20; ++i) {
    Random();
  }
  Initialize();
}

Symbol Context::Intern(const core::StringPiece& str) {
  return table_.Lookup(str);
}

Symbol Context::Intern(const core::UStringPiece& str) {
  return table_.Lookup(str);
}

Symbol Context::Intern(const Identifier& ident) {
  return ident.symbol();
}

Symbol Context::InternIndex(uint32_t index) {
  std::tr1::array<char, 15> buf;
  return table_.Lookup(
      core::StringPiece(
          buf.data(),
          std::snprintf(
              buf.data(), buf.size(), "%lu",
              static_cast<unsigned long>(index))));  // NOLINT
}

Symbol Context::InternDouble(double number) {
  std::tr1::array<char, 80> buffer;
  const char* const str = core::DoubleToCString(number,
                                                buffer.data(),
                                                buffer.size());
  return table_.Lookup(core::StringPiece(str));
}

Symbol Context::CheckIntern(const core::StringPiece& str,
                            bool* found) {
  return table_.LookupAndCheck(str, found);
}

Symbol Context::CheckIntern(const core::UStringPiece& str,
                            bool* found) {
  return table_.LookupAndCheck(str, found);
}

Symbol Context::CheckIntern(uint32_t index, bool* found) {
  std::tr1::array<char, 15> buf;
  return table_.LookupAndCheck(
      core::StringPiece(
          buf.data(),
          std::snprintf(
              buf.data(), buf.size(), "%lu",
              static_cast<unsigned long>(index))), found);  // NOLINT
}

Symbol Context::CheckIntern(double number, bool* found) {
  std::tr1::array<char, 80> buffer;
  const char* const str = core::DoubleToCString(number,
                                                buffer.data(),
                                                buffer.size());
  return table_.LookupAndCheck(core::StringPiece(str), found);
}

double Context::Random() {
  return random_engine_();
}

JSString* Context::ToString(Symbol sym) {
  return table_.ToString(this, sym);
}

const core::UString& Context::GetSymbolString(Symbol sym) const {
  return table_.GetSymbolString(sym);
}

bool Context::InCurrentLabelSet(
    const AnonymousBreakableStatement* stmt) const {
  // AnonymousBreakableStatement has empty label at first
  return !target_ || stmt == target_;
}

bool Context::InCurrentLabelSet(
    const NamedOnlyBreakableStatement* stmt) const {
  return stmt == target_;
}

bool Context::Run(JSScript* script) {
  const ScriptScope scope(this, script);
  interp_.Run(script->function(), script->type() == JSScript::kEval);
  assert(!ret_.IsEmpty() || error_);
  return error_;
}

JSVal Context::ErrorVal() {
  return JSError::Detail(this, &error_);
}

void Context::Initialize() {
  // Object and Function
  JSObject* const obj_proto = JSObject::NewPlain(this);

  // Function
  JSFunction* const func_proto =
      JSInlinedFunction<&runtime::FunctionPrototype, 0>::NewPlain(this);
  JSFunction* const func_constructor =
      JSInlinedFunction<&runtime::FunctionConstructor, 1>::NewPlain(this);
  func_proto->set_prototype(obj_proto);
  struct Class func_cls = {
    Intern("Function"),
    JSString::NewAsciiString(this, "Function"),
    func_constructor,
    func_proto
  };
  func_proto->set_class_name(func_cls.name);
  func_constructor->set_class_name(func_cls.name);
  func_constructor->set_prototype(func_cls.prototype);
  // set prototype
  func_constructor->DefineOwnProperty(
      this, prototype_symbol_,
      DataDescriptor(func_proto, PropertyDescriptor::NONE),
      false, NULL);

  builtins_[func_cls.name] = func_cls;

  global_obj_.DefineOwnProperty(
      this, func_cls.name,
      DataDescriptor(func_constructor,
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  func_proto->DefineOwnProperty(
      this, constructor_symbol_,
      DataDescriptor(func_constructor,
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  // section 15.2.2
  JSFunction* const obj_constructor =
      JSInlinedFunction<&runtime::ObjectConstructor, 1>::NewPlain(this);

  struct Class obj_cls = {
    Intern("Object"),
    JSString::NewAsciiString(this, "Object"),
    obj_constructor,
    obj_proto
  };
  obj_proto->set_class_name(obj_cls.name);
  builtins_[obj_cls.name] = obj_cls;

  // lazy initialization
  obj_constructor->set_class_name(func_cls.name);
  obj_constructor->set_prototype(func_cls.prototype);
  obj_constructor->DefineOwnProperty(
      this, prototype_symbol_,
      DataDescriptor(obj_proto, PropertyDescriptor::NONE),
      false, NULL);
  obj_proto->DefineOwnProperty(
      this, constructor_symbol_,
      DataDescriptor(obj_constructor,
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  func_proto->DefineOwnProperty(
      this, toString_symbol_,
      DataDescriptor(
          JSInlinedFunction<&runtime::FunctionToString, 0>::New(this),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  // section 15.3.4.3 Function.prototype.apply(thisArg, argArray)
  func_proto->DefineOwnProperty(
      this, Intern("apply"),
      DataDescriptor(
          JSInlinedFunction<&runtime::FunctionApply, 2>::New(this),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  // section 15.3.4.4 Function.prototype.call(thisArg[, arg1[, arg2, ...]])
  func_proto->DefineOwnProperty(
      this, Intern("call"),
      DataDescriptor(
          JSInlinedFunction<&runtime::FunctionCall, 1>::New(this),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  // section 15.3.4.5 Function.prototype.bind(thisArg[, arg1[, arg2, ...]])
  func_proto->DefineOwnProperty(
      this, Intern("bind"),
      DataDescriptor(
          JSInlinedFunction<&runtime::FunctionBind, 1>::New(this),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  {
    // Object Define

    // section 15.2.3.2 Object.getPrototypeOf(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("getPrototypeOf"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectGetPrototypeOf, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.3 Object.getOwnPropertyDescriptor(O, P)
    obj_constructor->DefineOwnProperty(
        this, Intern("getOwnPropertyDescriptor"),
        DataDescriptor(
            JSInlinedFunction<
              &runtime::ObjectGetOwnPropertyDescriptor, 2>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.4 Object.getOwnPropertyNames(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("getOwnPropertyNames"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectGetOwnPropertyNames, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.5 Object.create(O[, Properties])
    obj_constructor->DefineOwnProperty(
        this, Intern("create"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectCreate, 2>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.6 Object.defineProperty(O, P, Attributes)
    obj_constructor->DefineOwnProperty(
        this, Intern("defineProperty"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectDefineProperty, 3>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.7 Object.defineProperties(O, Properties)
    obj_constructor->DefineOwnProperty(
        this, Intern("defineProperties"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectDefineProperties, 2>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.8 Object.seal(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("seal"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectSeal, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.9 Object.freeze(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("freeze"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectFreeze, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.10 Object.preventExtensions(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("preventExtensions"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectPreventExtensions, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.11 Object.isSealed(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("isSealed"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectIsSealed, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.12 Object.isFrozen(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("isFrozen"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectIsFrozen, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.13 Object.isExtensible(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("isExtensible"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectIsExtensible, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.14 Object.keys(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("keys"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectKeys, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.2 Object.prototype.toString()
    obj_proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectToString, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.3 Object.prototype.toLocaleString()
    obj_proto->DefineOwnProperty(
        this, Intern("toLocaleString"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectToLocaleString, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.4 Object.prototype.valueOf()
    obj_proto->DefineOwnProperty(
        this, Intern("valueOf"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectValueOf, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.5 Object.prototype.hasOwnProperty(V)
    obj_proto->DefineOwnProperty(
        this, Intern("hasOwnProperty"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectHasOwnProperty, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.6 Object.prototype.isPrototypeOf(V)
    obj_proto->DefineOwnProperty(
        this, Intern("isPrototypeOf"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ObjectIsPrototypeOf, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.7 Object.prototype.propertyIsEnumerable(V)
    obj_proto->DefineOwnProperty(
        this, Intern("propertyIsEnumerable"),
        DataDescriptor(
            JSInlinedFunction<
              &runtime::ObjectPropertyIsEnumerable, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    global_obj_.DefineOwnProperty(
        this, obj_cls.name,
        DataDescriptor(obj_constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // Array
    JSObject* const proto = JSArray::NewPlain(this);
    // section 15.4.2 The Array Constructor
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::ArrayConstructor, 1>::NewPlain(this);
    constructor->set_class_name(func_cls.name);
    constructor->set_prototype(func_cls.prototype);

    // set prototype
    constructor->DefineOwnProperty(
        this, prototype_symbol_,
        DataDescriptor(proto, PropertyDescriptor::NONE),
        false, NULL);
    proto->set_prototype(obj_proto);
    struct Class cls = {
      Array_symbol_,
      JSString::NewAsciiString(this, "Array"),
      constructor,
      proto
    };
    proto->set_class_name(cls.name);

    builtins_[cls.name] = cls;
    global_obj_.DefineOwnProperty(
        this, cls.name,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    proto->DefineOwnProperty(
        this, constructor_symbol_,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.3.2 Array.isArray(arg)
    constructor->DefineOwnProperty(
        this, Intern("isArray"),
        DataDescriptor(
            JSInlinedFunction<&runtime::ArrayIsArray, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    bind::Object(this, proto)
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
        // section 15.4.4.15 Array.prototype.lastIndexOf(searchElement[, fromIndex])
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
        // section 15.4.4.22 Array.prototype.reduceRight(callbackfn[, initialValue])
        .def<&runtime::ArrayReduceRight, 1>("reduceRight");
  }

  {
    // String
    JSStringObject* const proto = JSStringObject::NewPlain(this);

    // section 15.5.2 The String Constructor
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::StringConstructor, 1>::NewPlain(this);
    constructor->set_class_name(func_cls.name);
    constructor->set_prototype(func_cls.prototype);

    // set prototype
    constructor->DefineOwnProperty(
        this, prototype_symbol_,
        DataDescriptor(proto, PropertyDescriptor::NONE),
        false, NULL);
    proto->set_prototype(obj_proto);
    struct Class cls = {
      Intern("String"),
      JSString::NewAsciiString(this, "String"),
      constructor,
      proto
    };
    proto->set_class_name(cls.name);

    builtins_[cls.name] = cls;
    global_obj_.DefineOwnProperty(
        this, cls.name,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    proto->DefineOwnProperty(
        this, constructor_symbol_,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.3.2 String.fromCharCode([char0 [, char1[, ...]]])
    constructor->DefineOwnProperty(
        this, Intern("fromCharCode"),
        DataDescriptor(
            JSInlinedFunction<&runtime::StringFromCharCode, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    bind::Object(this, proto)
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
        // section B.2.3 String.prototype.substr(start, length)
        // this method is deprecated.
        .def<&runtime::StringSubstr, 2>("substr");
  }

  {
    // Boolean
    JSBooleanObject* const proto = JSBooleanObject::NewPlain(this, false);

    // section 15.5.2 The Boolean Constructor
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::BooleanConstructor, 1>::NewPlain(this);
    constructor->set_class_name(func_cls.name);
    constructor->set_prototype(func_cls.prototype);

    // set prototype
    constructor->DefineOwnProperty(
        this, prototype_symbol_,
        DataDescriptor(proto, PropertyDescriptor::NONE),
        false, NULL);
    proto->set_prototype(obj_proto);
    struct Class cls = {
      Intern("Boolean"),
      JSString::NewAsciiString(this, "Boolean"),
      constructor,
      proto
    };
    proto->set_class_name(cls.name);

    builtins_[cls.name] = cls;
    global_obj_.DefineOwnProperty(
        this, cls.name,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    proto->DefineOwnProperty(
        this, constructor_symbol_,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.6.4.2 Boolean.prototype.toString()
    proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(
            JSInlinedFunction<&runtime::BooleanToString, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.6.4.3 Boolean.prototype.valueOf()
    proto->DefineOwnProperty(
        this, valueOf_symbol_,
        DataDescriptor(
            JSInlinedFunction<&runtime::BooleanValueOf, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // Number
    JSNumberObject* const proto = JSNumberObject::NewPlain(this, 0);

    // section 15.7.3 The Number Constructor
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::NumberConstructor, 1>::NewPlain(this);
    constructor->set_class_name(func_cls.name);
    constructor->set_prototype(func_cls.prototype);

    // set prototype
    constructor->DefineOwnProperty(
        this, prototype_symbol_,
        DataDescriptor(proto, PropertyDescriptor::NONE),
        false, NULL);
    proto->set_prototype(obj_proto);
    struct Class cls = {
      Intern("Number"),
      JSString::NewAsciiString(this, "Number"),
      constructor,
      proto
    };
    proto->set_class_name(cls.name);

    builtins_[cls.name] = cls;
    global_obj_.DefineOwnProperty(
        this, cls.name,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.3.2 Number.MAX_VALUE
    constructor->DefineOwnProperty(
        this, Intern("MAX_VALUE"),
        DataDescriptor(1.7976931348623157e+308,
                       PropertyDescriptor::NONE),
        false, NULL);

    // section 15.7.3.3 Number.MIN_VALUE
    constructor->DefineOwnProperty(
        this, Intern("MIN_VALUE"),
        DataDescriptor(5e-324,
                       PropertyDescriptor::NONE),
        false, NULL);

    // section 15.7.3.4 Number.NaN
    constructor->DefineOwnProperty(
        this, Intern("NaN"),
        DataDescriptor(JSValData::kNaN,
                       PropertyDescriptor::NONE),
        false, NULL);

    // section 15.7.3.5 Number.NEGATIVE_INFINITY
    constructor->DefineOwnProperty(
        this, Intern("NEGATIVE_INFINITY"),
        DataDescriptor(-std::numeric_limits<double>::infinity(),
                       PropertyDescriptor::NONE),
        false, NULL);

    // section 15.7.3.6 Number.POSITIVE_INFINITY
    constructor->DefineOwnProperty(
        this, Intern("POSITIVE_INFINITY"),
        DataDescriptor(std::numeric_limits<double>::infinity(),
                       PropertyDescriptor::NONE),
        false, NULL);

    // section 15.7.4.1 Number.prototype.constructor
    proto->DefineOwnProperty(
        this, constructor_symbol_,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.2 Number.prototype.toString([radix])
    proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(
            JSInlinedFunction<&runtime::NumberToString, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.3 Number.prototype.toLocaleString()
    proto->DefineOwnProperty(
        this, Intern("toLocaleString"),
        DataDescriptor(
            JSInlinedFunction<&runtime::NumberToLocaleString, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.4 Number.prototype.valueOf()
    proto->DefineOwnProperty(
        this, valueOf_symbol_,
        DataDescriptor(
            JSInlinedFunction<&runtime::NumberValueOf, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.5 Number.prototype.toFixed(fractionDigits)
    proto->DefineOwnProperty(
        this, Intern("toFixed"),
        DataDescriptor(
          JSInlinedFunction<&runtime::NumberToFixed, 1>::New(this),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.6 Number.prototype.toExponential(fractionDigits)
    proto->DefineOwnProperty(
        this, Intern("toExponential"),
        DataDescriptor(
          JSInlinedFunction<&runtime::NumberToExponential, 1>::New(this),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.7 Number.prototype.toPrecision(precision)
    proto->DefineOwnProperty(
        this, Intern("toPrecision"),
        DataDescriptor(
          JSInlinedFunction<&runtime::NumberToPrecision, 1>::New(this),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // Error
    JSObject* const proto = JSObject::NewPlain(this);
    // section 15.11.2 The Error Constructor
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::ErrorConstructor, 1>::NewPlain(this);
    constructor->set_class_name(func_cls.name);
    constructor->set_prototype(func_cls.prototype);
    // set prototype
    constructor->DefineOwnProperty(
        this, prototype_symbol_,
        DataDescriptor(proto, PropertyDescriptor::NONE),
        false, NULL);
    proto->set_prototype(obj_proto);
    struct Class cls = {
      Intern("Error"),
      JSString::NewAsciiString(this, "Error"),
      constructor,
      proto
    };
    proto->set_class_name(cls.name);
    builtins_[cls.name] = cls;
    proto->DefineOwnProperty(
        this, constructor_symbol_,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.11.4.4 Error.prototype.toString()
    proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(
            JSInlinedFunction<&runtime::ErrorToString, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.11.4.2 Error.prototype.name
    proto->DefineOwnProperty(
        this, Intern("name"),
        DataDescriptor(JSString::NewAsciiString(this, "Error"),
                       PropertyDescriptor::NONE),
        false, NULL);

    // section 15.11.4.3 Error.prototype.message
    proto->DefineOwnProperty(
        this, Intern("message"),
        DataDescriptor(JSString::NewEmptyString(this),
                       PropertyDescriptor::NONE),
        false, NULL);

    global_obj_.DefineOwnProperty(
        this, cls.name,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    {
      // section 15.11.6.1 EvalError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSFunction* const sub_constructor =
          JSInlinedFunction<&runtime::EvalErrorConstructor, 1>::NewPlain(this);
      const Symbol sym = Intern("EvalError");
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("Error"),
        JSString::NewAsciiString(this, "EvalError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sym] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sym,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
      sub_proto->DefineOwnProperty(
          this, Intern("name"),
          DataDescriptor(
              JSString::NewAsciiString(this, "EvalError"),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("message"),
          DataDescriptor(
              JSString::NewEmptyString(this),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.2 RangeError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSFunction* const sub_constructor =
          JSInlinedFunction<&runtime::RangeErrorConstructor, 1>::NewPlain(this);
      const Symbol sym = Intern("RangeError");
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("Error"),
        JSString::NewAsciiString(this, "RangeError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sym] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sym,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("name"),
          DataDescriptor(
              JSString::NewAsciiString(this, "RangeError"),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("message"),
          DataDescriptor(
              JSString::NewEmptyString(this),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.3 ReferenceError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSFunction* const sub_constructor =
          JSInlinedFunction<&runtime::ReferenceErrorConstructor, 1>::NewPlain(this);
      const Symbol sym = Intern("ReferenceError");
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("Error"),
        JSString::NewAsciiString(this, "ReferenceError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sym] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sym,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("name"),
          DataDescriptor(
              JSString::NewAsciiString(this, "ReferenceError"),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("message"),
          DataDescriptor(
              JSString::NewEmptyString(this),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.4 SyntaxError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSFunction* const sub_constructor =
          JSInlinedFunction<&runtime::SyntaxErrorConstructor, 1>::NewPlain(this);
      const Symbol sym = Intern("SyntaxError");
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("Error"),
        JSString::NewAsciiString(this, "SyntaxError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sym] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sym,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("name"),
          DataDescriptor(
              JSString::NewAsciiString(this, "SyntaxError"),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("message"),
          DataDescriptor(
              JSString::NewEmptyString(this),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.5 TypeError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSFunction* const sub_constructor =
          JSInlinedFunction<&runtime::TypeErrorConstructor, 1>::NewPlain(this);
      const Symbol sym = Intern("TypeError");
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("Error"),
        JSString::NewAsciiString(this, "TypeError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sym] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sym,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("name"),
          DataDescriptor(
              JSString::NewAsciiString(this, "TypeError"),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("message"),
          DataDescriptor(
              JSString::NewEmptyString(this),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.6 URIError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSFunction* const sub_constructor =
          JSInlinedFunction<&runtime::URIErrorConstructor, 1>::NewPlain(this);
      const Symbol sym = Intern("URIError");
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("Error"),
        JSString::NewAsciiString(this, "URIError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sym] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sym,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("name"),
          DataDescriptor(
              JSString::NewAsciiString(this, "URIError"),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, Intern("message"),
          DataDescriptor(
              JSString::NewEmptyString(this),
              PropertyDescriptor::NONE),
          false, NULL);

      sub_proto->DefineOwnProperty(
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
  }

  {
    // section 15.8 Math
    JSObject* const math = JSObject::NewPlain(this);
    global_obj_.DefineOwnProperty(
        this, Intern("Math"),
        DataDescriptor(math,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    bind::Object(this, math)
        .class_name("Math")
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
        .def<&runtime::MathTan, 1>("tan");
  }
  {
    // section 15.9 Date
    JSObject* const proto = JSDate::NewPlain(this, JSValData::kNaN);
    // section 15.9.2.1 The Date Constructor
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::DateConstructor, 7>::NewPlain(this);
    constructor->set_class_name(func_cls.name);
    constructor->set_prototype(func_cls.prototype);

    // set prototype
    constructor->DefineOwnProperty(
        this, prototype_symbol_,
        DataDescriptor(proto, PropertyDescriptor::NONE),
        false, NULL);
    proto->set_prototype(obj_proto);
    struct Class cls = {
      Intern("Date"),
      JSString::NewAsciiString(this, "Date"),
      constructor,
      proto
    };
    proto->set_class_name(cls.name);

    builtins_[cls.name] = cls;

    global_obj_.DefineOwnProperty(
        this, cls.name,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    proto->DefineOwnProperty(
        this, constructor_symbol_,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.4.2 Date.parse(string)
    constructor->DefineOwnProperty(
        this, Intern("parse"),
        DataDescriptor(
            JSInlinedFunction<&runtime::DateParse, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.4.3 Date.UTC()
    constructor->DefineOwnProperty(
        this, Intern("UTC"),
        DataDescriptor(
            JSInlinedFunction<&runtime::DateUTC, 7>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.4.4 Date.now()
    constructor->DefineOwnProperty(
        this, Intern("now"),
        DataDescriptor(
            JSInlinedFunction<&runtime::DateNow, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    bind::Object(this, proto)
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
        .def<&runtime::DateSetUTCFullYear, 3>("setUTCFullYear");

    JSFunction* const toUTCString =
        JSInlinedFunction<&runtime::DateToUTCString, 0>::New(this);

    // section 15.9.5.42 Date.prototype.toUTCString()
    proto->DefineOwnProperty(
        this, Intern("toUTCString"),
        DataDescriptor(
            toUTCString,
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.43 Date.prototype.toISOString()
    proto->DefineOwnProperty(
        this, Intern("toISOString"),
        DataDescriptor(
            JSInlinedFunction<&runtime::DateToISOString, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.44 Date.prototype.toJSON()
    proto->DefineOwnProperty(
        this, Intern("toJSON"),
        DataDescriptor(
            JSInlinedFunction<&runtime::DateToJSON, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section B.2.4 Date.prototype.getYear()
    // this method is deprecated.
    proto->DefineOwnProperty(
        this, Intern("getYear"),
        DataDescriptor(
            JSInlinedFunction<&runtime::DateGetYear, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section B.2.5 Date.prototype.setYear(year)
    // this method is deprecated.
    proto->DefineOwnProperty(
        this, Intern("setYear"),
        DataDescriptor(
            JSInlinedFunction<&runtime::DateSetYear, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section B.2.6 Date.prototype.toGMTString()
    proto->DefineOwnProperty(
        this, Intern("toGMTString"),
        DataDescriptor(
            toUTCString,
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // RegExp
    JSObject* const proto = JSRegExp::NewPlain(this);
    // section 15.10.4 The RegExp Constructor
    JSFunction* const constructor =
        JSInlinedFunction<&runtime::RegExpConstructor, 2>::NewPlain(this);
    constructor->set_class_name(func_cls.name);
    constructor->set_prototype(func_cls.prototype);

    // set prototype
    constructor->DefineOwnProperty(
        this, prototype_symbol_,
        DataDescriptor(proto, PropertyDescriptor::NONE),
        false, NULL);
    proto->set_prototype(obj_proto);
    struct Class cls = {
      Intern("RegExp"),
      JSString::NewAsciiString(this, "RegExp"),
      constructor,
      proto
    };
    proto->set_class_name(cls.name);

    builtins_[cls.name] = cls;
    global_obj_.DefineOwnProperty(
        this, cls.name,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    proto->DefineOwnProperty(
        this, constructor_symbol_,
        DataDescriptor(constructor,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.10.6.2 RegExp.prototype.exec(string)
    proto->DefineOwnProperty(
        this, Intern("exec"),
        DataDescriptor(
            JSInlinedFunction<&runtime::RegExpExec, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.10.6.3 RegExp.prototype.test(string)
    proto->DefineOwnProperty(
        this, Intern("test"),
        DataDescriptor(
            JSInlinedFunction<&runtime::RegExpTest, 1>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.10.6.4 RegExp.prototype.toString()
    proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(
            JSInlinedFunction<&runtime::RegExpToString, 0>::New(this),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // section 15.12 JSON
    JSObject* const json = JSObject::NewPlain(this);
    global_obj_.DefineOwnProperty(
        this, Intern("JSON"),
        DataDescriptor(json,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    bind::Object(this, json)
        .class_name("JSON")
        .prototype(obj_proto)
        // section 15.12.2 parse(text[, reviver])
        .def<&runtime::JSONParse, 2>("parse")
        // section 15.12.3 stringify(value[, replacer[, space]])
        .def<&runtime::JSONStringify, 3>("stringify");
  }

  {
    // Global
    bind::Object(this, &global_obj_)
        .class_name("global")
        .prototype(obj_proto)
        // section 15.1.1.1 NaN
        .def("NaN", JSValData::kNaN)
        // section 15.1.1.2 Infinity
        .def("Infinity", std::numeric_limits<double>::infinity())
        // section 15.1.1.3 undefined
        .def("undefined", JSUndefined)
        // section 15.1.2.1 eval(x)
        .def<&runtime::GlobalEval, 1>("eval")
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

  {
    // Arguments
    struct Class cls = {
      Intern("Arguments"),
      JSString::NewAsciiString(this, "Arguments"),
      NULL,
      obj_proto
    };
    builtins_[cls.name] = cls;
  }
  throw_type_error_.Initialize(this);
}

const Class& Context::Cls(Symbol name) {
  assert(builtins_.find(name) != builtins_.end());
  return builtins_[name];
}

const Class& Context::Cls(const core::StringPiece& str) {
  assert(builtins_.find(Intern(str)) != builtins_.end());
  return builtins_[Intern(str)];
}

} }  // namespace iv::lv5
