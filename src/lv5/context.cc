#include <cmath>
#include <cstdio>
#include <tr1/array>
#include "cstring"
#include "ustring.h"
#include "jsfunction.h"
#include "arguments.h"
#include "context.h"
#include "dtoa.h"
#include "context_utils.h"
#include "interpreter.h"
#include "property.h"
#include "class.h"
#include "runtime.h"
#include "jsast.h"
#include "jsscript.h"
#include "jserror.h"
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

}  // namespace iv::lv5::context

Context::Context()
  : global_obj_(),
    throw_type_error_(),
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
    current_script_(NULL) {
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
  JSNativeFunction* const func_proto =
      JSNativeFunction::NewPlain(this, &runtime::FunctionPrototype, 0);
  JSNativeFunction* const func_constructor =
      JSNativeFunction::NewPlain(this, &runtime::FunctionConstructor, 1);
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
  JSNativeFunction* const obj_constructor =
      JSNativeFunction::NewPlain(this, &runtime::ObjectConstructor, 1);

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
      DataDescriptor(JSNativeFunction::New(this, &runtime::FunctionToString, 0),
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  // section 15.3.4.3 Function.prototype.apply(thisArg, argArray)
  func_proto->DefineOwnProperty(
      this, Intern("apply"),
      DataDescriptor(JSNativeFunction::New(this, &runtime::FunctionApply, 2),
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  // section 15.3.4.4 Function.prototype.call(thisArg[, arg1[, arg2, ...]])
  func_proto->DefineOwnProperty(
      this, Intern("call"),
      DataDescriptor(JSNativeFunction::New(this, &runtime::FunctionCall, 1),
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  // section 15.3.4.5 Function.prototype.bind(thisArg[, arg1[, arg2, ...]])
  func_proto->DefineOwnProperty(
      this, Intern("bind"),
      DataDescriptor(JSNativeFunction::New(this, &runtime::FunctionBind, 1),
                     PropertyDescriptor::WRITABLE |
                     PropertyDescriptor::CONFIGURABLE),
      false, NULL);

  {
    // Object Define

    // section 15.2.3.2 Object.getPrototypeOf(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("getPrototypeOf"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectGetPrototypeOf, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.3 Object.getOwnPropertyDescriptor(O, P)
    obj_constructor->DefineOwnProperty(
        this, Intern("getOwnPropertyDescriptor"),
        DataDescriptor(
            JSNativeFunction::New(this,
                                  &runtime::ObjectGetOwnPropertyDescriptor, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.4 Object.getOwnPropertyNames(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("getOwnPropertyNames"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectGetOwnPropertyNames, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.5 Object.create(O[, Properties])
    obj_constructor->DefineOwnProperty(
        this, Intern("create"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectCreate, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.6 Object.defineProperty(O, P, Attributes)
    obj_constructor->DefineOwnProperty(
        this, Intern("defineProperty"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectDefineProperty, 3),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.7 Object.defineProperties(O, Properties)
    obj_constructor->DefineOwnProperty(
        this, Intern("defineProperties"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectDefineProperties, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.8 Object.seal(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("seal"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectSeal, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.9 Object.freeze(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("freeze"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectFreeze, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.10 Object.preventExtensions(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("preventExtensions"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectPreventExtensions, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.11 Object.isSealed(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("isSealed"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectIsSealed, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.12 Object.isFrozen(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("isFrozen"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectIsFrozen, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.13 Object.isExtensible(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("isExtensible"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectIsExtensible, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.3.14 Object.keys(O)
    obj_constructor->DefineOwnProperty(
        this, Intern("keys"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectKeys, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.2 Object.prototype.toString()
    obj_proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(JSNativeFunction::New(this, &runtime::ObjectToString, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.3 Object.prototype.toLocaleString()
    obj_proto->DefineOwnProperty(
        this, Intern("toLocaleString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectToLocaleString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.4 Object.prototype.valueOf()
    obj_proto->DefineOwnProperty(
        this, Intern("valueOf"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectValueOf, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.5 Object.prototype.hasOwnProperty(V)
    obj_proto->DefineOwnProperty(
        this, Intern("hasOwnProperty"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectHasOwnProperty, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.6 Object.prototype.isPrototypeOf(V)
    obj_proto->DefineOwnProperty(
        this, Intern("isPrototypeOf"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ObjectIsPrototypeOf, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.2.4.7 Object.prototype.propertyIsEnumerable(V)
    obj_proto->DefineOwnProperty(
        this, Intern("propertyIsEnumerable"),
        DataDescriptor(
            JSNativeFunction::New(
                this, &runtime::ObjectPropertyIsEnumerable, 1),
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
    JSNativeFunction* const constructor =
        JSNativeFunction::NewPlain(this, &runtime::ArrayConstructor, 1);
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
            JSNativeFunction::New(this, &runtime::ArrayIsArray, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.2 Array.prototype.toString()
    proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayToString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.3 Array.prototype.toLocaleString()
    proto->DefineOwnProperty(
        this, Intern("toLocaleString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayToLocaleString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.4 Array.prototype.concat([item1[, item2[, ...]]])
    proto->DefineOwnProperty(
        this, Intern("concat"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayConcat, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.5 Array.prototype.join(separator)
    proto->DefineOwnProperty(
        this, Intern("join"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayJoin, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.6 Array.prototype.pop()
    proto->DefineOwnProperty(
        this, Intern("pop"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayPop, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.7 Array.prototype.push([item1[, item2[, ...]]])
    proto->DefineOwnProperty(
        this, Intern("push"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayPush, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.8 Array.prototype.reverse()
    proto->DefineOwnProperty(
        this, Intern("reverse"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayReverse, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.9 Array.prototype.shift()
    proto->DefineOwnProperty(
        this, Intern("shift"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayShift, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.10 Array.prototype.slice(start, end)
    proto->DefineOwnProperty(
        this, Intern("slice"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArraySlice, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.12 Array.prototype.splice(start, deleteCount[, item1[, item2[, ...]]])  // NOLINT
    proto->DefineOwnProperty(
        this, Intern("splice"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArraySplice, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.13 Array.prototype.unshift([item1[, item2[, ...]]])
    proto->DefineOwnProperty(
        this, Intern("unshift"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayUnshift, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.14 Array.prototype.indexOf(searchElement[, fromIndex])
    proto->DefineOwnProperty(
        this, Intern("indexOf"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayIndexOf, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.15 Array.prototype.lastIndexOf(searchElement[, fromIndex])
    proto->DefineOwnProperty(
        this, Intern("lastIndexOf"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayLastIndexOf, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.16 Array.prototype.every(callbackfn[, thisArg])
    proto->DefineOwnProperty(
        this, Intern("every"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayEvery, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.17 Array.prototype.some(callbackfn[, thisArg])
    proto->DefineOwnProperty(
        this, Intern("some"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArraySome, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.18 Array.prototype.forEach(callbackfn[, thisArg])
    proto->DefineOwnProperty(
        this, Intern("forEach"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayForEach, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.19 Array.prototype.map(callbackfn[, thisArg])
    proto->DefineOwnProperty(
        this, Intern("map"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayMap, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.20 Array.prototype.filter(callbackfn[, thisArg])
    proto->DefineOwnProperty(
        this, Intern("filter"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayFilter, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.21 Array.prototype.reduce(callbackfn[, initialValue])
    proto->DefineOwnProperty(
        this, Intern("reduce"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayReduce, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.4.4.22 Array.prototype.reduceRight(callbackfn[, initialValue])
    proto->DefineOwnProperty(
        this, Intern("reduceRight"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::ArrayReduceRight, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // String
    // TODO(Constellation) more
    JSStringObject* const proto = JSStringObject::NewPlain(this);

    // section 15.5.2 The String Constructor
    JSNativeFunction* const constructor =
        JSNativeFunction::NewPlain(this, &runtime::StringConstructor, 1);
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
            JSNativeFunction::New(this, &runtime::StringFromCharCode, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.2 String.prototype.toString()
    proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringToString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.3 String.prototype.valueOf()
    proto->DefineOwnProperty(
        this, valueOf_symbol_,
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringValueOf, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.4 String.prototype.charAt(pos)
    proto->DefineOwnProperty(
        this, Intern("charAt"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringCharAt, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.5 String.prototype.charCodeAt(pos)
    proto->DefineOwnProperty(
        this, Intern("charCodeAt"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringCharCodeAt, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.6 String.prototype.concat([string1[, string2[, ...]]])
    proto->DefineOwnProperty(
        this, Intern("concat"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringConcat, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.7 String.prototype.indexOf(searchString, position)
    proto->DefineOwnProperty(
        this, Intern("indexOf"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringIndexOf, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.8 String.prototype.lastIndexOf(searchString, position)
    proto->DefineOwnProperty(
        this, Intern("lastIndexOf"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringLastIndexOf, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.9 String.prototype.localeCompare(that)
    proto->DefineOwnProperty(
        this, Intern("localeCompare"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringLocaleCompare, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.10 String.prototype.match(regexp)
    proto->DefineOwnProperty(
        this, Intern("match"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringMatch, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.11 String.prototype.replace(searchValue, replaceValue)
    proto->DefineOwnProperty(
        this, Intern("replace"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringReplace, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.12 String.prototype.search(regexp)
    proto->DefineOwnProperty(
        this, Intern("search"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringSearch, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.13 String.prototype.slice(start, end)
    proto->DefineOwnProperty(
        this, Intern("slice"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringSlice, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.14 String.prototype.split(separator, limit)
    proto->DefineOwnProperty(
        this, Intern("split"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringSplit, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.15 String.prototype.substring(start, end)
    proto->DefineOwnProperty(
        this, Intern("substring"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringSubString, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.16 String.prototype.toLowerCase()
    proto->DefineOwnProperty(
        this, Intern("toLowerCase"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringToLowerCase, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.17 String.prototype.toLocaleLowerCase()
    proto->DefineOwnProperty(
        this, Intern("toLocaleLowerCase"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringToLocaleLowerCase, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.18 String.prototype.toUpperCase()
    proto->DefineOwnProperty(
        this, Intern("toUpperCase"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringToUpperCase, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.19 String.prototype.toLocaleUpperCase()
    proto->DefineOwnProperty(
        this, Intern("toLocaleUpperCase"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringToLocaleUpperCase, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.5.4.20 String.prototype.trim()
    proto->DefineOwnProperty(
        this, Intern("trim"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::StringTrim, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // Boolean
    JSBooleanObject* const proto = JSBooleanObject::NewPlain(this, false);

    // section 15.5.2 The Boolean Constructor
    JSNativeFunction* const constructor =
        JSNativeFunction::NewPlain(this, &runtime::BooleanConstructor, 1);
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
            JSNativeFunction::New(this, &runtime::BooleanToString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.6.4.3 Boolean.prototype.valueOf()
    proto->DefineOwnProperty(
        this, valueOf_symbol_,
        DataDescriptor(JSNativeFunction::New(this, &runtime::BooleanValueOf, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // Number
    JSNumberObject* const proto = JSNumberObject::NewPlain(this, 0);

    // section 15.7.3 The Number Constructor
    JSNativeFunction* const constructor =
        JSNativeFunction::NewPlain(this, &runtime::NumberConstructor, 1);
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
        DataDescriptor(JSNativeFunction::New(this, &runtime::NumberToString, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.3 Number.prototype.toLocaleString()
    proto->DefineOwnProperty(
        this, Intern("toLocaleString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::NumberToLocaleString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.4 Number.prototype.valueOf()
    proto->DefineOwnProperty(
        this, valueOf_symbol_,
        DataDescriptor(JSNativeFunction::New(this, &runtime::NumberValueOf, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.5 Number.prototype.toFixed(fractionDigits)
    proto->DefineOwnProperty(
        this, Intern("toFixed"),
        DataDescriptor(
          JSNativeFunction::New(this, &runtime::NumberToFixed, 1),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.6 Number.prototype.toExponential(fractionDigits)
    proto->DefineOwnProperty(
        this, Intern("toExponential"),
        DataDescriptor(
          JSNativeFunction::New(this, &runtime::NumberToExponential, 1),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.7.4.7 Number.prototype.toPrecision(precision)
    proto->DefineOwnProperty(
        this, Intern("toPrecision"),
        DataDescriptor(
          JSNativeFunction::New(this, &runtime::NumberToPrecision, 1),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // Error
    JSObject* const proto = JSObject::NewPlain(this);
    // section 15.11.2 The Error Constructor
    JSNativeFunction* const constructor =
        JSNativeFunction::NewPlain(this, &runtime::ErrorConstructor, 1);
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
        DataDescriptor(JSNativeFunction::New(this, &runtime::ErrorToString, 0),
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
        DataDescriptor(JSString::NewAsciiString(this, ""),
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
      JSNativeFunction* const sub_constructor =
          JSNativeFunction::NewPlain(this, &runtime::EvalErrorConstructor, 1);
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("EvalError"),
        JSString::NewAsciiString(this, "EvalError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sub_cls.name] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sub_cls.name,
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
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.2 RangeError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSNativeFunction* const sub_constructor =
          JSNativeFunction::NewPlain(this, &runtime::RangeErrorConstructor, 1);
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("RangeError"),
        JSString::NewAsciiString(this, "RangeError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sub_cls.name] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sub_cls.name,
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
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.3 ReferenceError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSNativeFunction* const sub_constructor =
          JSNativeFunction::NewPlain(this,
                                     &runtime::ReferenceErrorConstructor, 1);
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("ReferenceError"),
        JSString::NewAsciiString(this, "ReferenceError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sub_cls.name] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sub_cls.name,
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
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.4 SyntaxError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSNativeFunction* const sub_constructor =
          JSNativeFunction::NewPlain(this, &runtime::SyntaxErrorConstructor, 1);
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("SyntaxError"),
        JSString::NewAsciiString(this, "SyntaxError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sub_cls.name] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sub_cls.name,
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
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.5 TypeError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSNativeFunction* const sub_constructor =
          JSNativeFunction::NewPlain(this, &runtime::TypeErrorConstructor, 1);
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("TypeError"),
        JSString::NewAsciiString(this, "TypeError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sub_cls.name] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sub_cls.name,
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
          this, constructor_symbol_,
          DataDescriptor(sub_constructor,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, NULL);
    }
    {
      // section 15.11.6.6 URIError
      JSObject* const sub_proto = JSObject::NewPlain(this);
      JSNativeFunction* const sub_constructor =
          JSNativeFunction::NewPlain(this, &runtime::URIErrorConstructor, 1);
      sub_constructor->set_class_name(func_cls.name);
      sub_constructor->set_prototype(func_cls.prototype);
      // set prototype
      sub_constructor->DefineOwnProperty(
          this, prototype_symbol_,
          DataDescriptor(sub_proto, PropertyDescriptor::NONE),
          false, NULL);
      sub_proto->set_prototype(proto);
      struct Class sub_cls = {
        Intern("URIError"),
        JSString::NewAsciiString(this, "URIError"),
        sub_constructor,
        sub_proto
      };
      sub_proto->set_class_name(sub_cls.name);
      builtins_[sub_cls.name] = sub_cls;
      global_obj_.DefineOwnProperty(
          this, sub_cls.name,
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
    math->set_prototype(obj_proto);
    const Symbol name = Intern("Math");
    math->set_class_name(name);
    global_obj_.DefineOwnProperty(
        this, name,
        DataDescriptor(math,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.1.1 E
    math->DefineOwnProperty(
        this, Intern("E"),
        DataDescriptor(std::exp(1.0), PropertyDescriptor::NONE),
        false, NULL);

    // section 15.8.1.2 LN10
    math->DefineOwnProperty(
        this, Intern("LN10"),
        DataDescriptor(std::log(10.0), PropertyDescriptor::NONE),
        false, NULL);

    // section 15.8.1.3 LN2
    math->DefineOwnProperty(
        this, Intern("LN2"),
        DataDescriptor(std::log(2.0), PropertyDescriptor::NONE),
        false, NULL);

    // section 15.8.1.4 LOG2E
    math->DefineOwnProperty(
        this, Intern("LOG2E"),
        DataDescriptor(1.0 / std::log(2.0), PropertyDescriptor::NONE),
        false, NULL);

    // section 15.8.1.5 LOG10E
    math->DefineOwnProperty(
        this, Intern("LOG10E"),
        DataDescriptor(1.0 / std::log(10.0), PropertyDescriptor::NONE),
        false, NULL);

    // section 15.8.1.6 PI
    math->DefineOwnProperty(
        this, Intern("PI"),
        DataDescriptor(std::acos(-1.0), PropertyDescriptor::NONE),
        false, NULL);

    // section 15.8.1.7 SQRT1_2
    math->DefineOwnProperty(
        this, Intern("SQRT1_2"),
        DataDescriptor(std::sqrt(0.5), PropertyDescriptor::NONE),
        false, NULL);

    // section 15.8.1.8 SQRT2
    math->DefineOwnProperty(
        this, Intern("SQRT2"),
        DataDescriptor(std::sqrt(2.0), PropertyDescriptor::NONE),
        false, NULL);

    // section 15.8.2.1 abs(x)
    math->DefineOwnProperty(
        this, Intern("abs"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathAbs, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.2 acos(x)
    math->DefineOwnProperty(
        this, Intern("acos"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathAcos, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.3 asin(x)
    math->DefineOwnProperty(
        this, Intern("asin"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathAsin, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.4 atan(x)
    math->DefineOwnProperty(
        this, Intern("atan"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathAtan, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.5 atan2(y, x)
    math->DefineOwnProperty(
        this, Intern("atan2"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathAtan2, 2),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.6 ceil(x)
    math->DefineOwnProperty(
        this, Intern("ceil"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathCeil, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.7 cos(x)
    math->DefineOwnProperty(
        this, Intern("cos"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathCos, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.8 exp(x)
    math->DefineOwnProperty(
        this, Intern("exp"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathExp, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.9 floor(x)
    math->DefineOwnProperty(
        this, Intern("floor"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathFloor, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.10 log(x)
    math->DefineOwnProperty(
        this, Intern("log"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathLog, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.11 max([value1[, value2[, ... ]]])
    math->DefineOwnProperty(
        this, Intern("max"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathMax, 2),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.12 min([value1[, value2[, ... ]]])
    math->DefineOwnProperty(
        this, Intern("min"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathMin, 2),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.13 pow(x, y)
    math->DefineOwnProperty(
        this, Intern("pow"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathPow, 2),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.14 random()
    math->DefineOwnProperty(
        this, Intern("random"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathRandom, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.15 round(x)
    math->DefineOwnProperty(
        this, Intern("round"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathRound, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.16 sin(x)
    math->DefineOwnProperty(
        this, Intern("sin"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathSin, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.17 sqrt(x)
    math->DefineOwnProperty(
        this, Intern("sqrt"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathSqrt, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.8.2.18 tan(x)
    math->DefineOwnProperty(
        this, Intern("tan"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::MathTan, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }
  {
    // section 15.9 Date
    JSObject* const proto = JSDate::NewPlain(this, JSValData::kNaN);
    // section 15.9.2.1 The Date Constructor
    JSNativeFunction* const constructor =
        JSNativeFunction::NewPlain(this, &runtime::DateConstructor, 7);
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

    // section 15.9.4.3 Date.UTC()
    constructor->DefineOwnProperty(
        this, Intern("UTC"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateUTC, 7),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.4.4 Date.now()
    constructor->DefineOwnProperty(
        this, Intern("now"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateNow, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.2 Date.prototype.toString()
    proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateToString, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.3 Date.prototype.toDateString()
    proto->DefineOwnProperty(
        this, Intern("toDateString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateToDateString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.4 Date.prototype.toTimeString()
    proto->DefineOwnProperty(
        this, Intern("toTimeString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateToTimeString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.5 Date.prototype.toLocaleString()
    proto->DefineOwnProperty(
        this, Intern("toLocaleString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateToLocaleString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.6 Date.prototype.toLocaleDateString()
    proto->DefineOwnProperty(
        this, Intern("toLocaleDateString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateToLocaleDateString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.7 Date.prototype.toLocaleTimeString()
    proto->DefineOwnProperty(
        this, Intern("toLocaleTimeString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateToLocaleTimeString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.8 Date.prototype.valueOf()
    proto->DefineOwnProperty(
        this, Intern("valueOf"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateValueOf, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.9 Date.prototype.getTime()
    proto->DefineOwnProperty(
        this, Intern("getTime"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateGetTime, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.10 Date.prototype.getFullYear()
    proto->DefineOwnProperty(
        this, Intern("getFullYear"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetFullYear, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.11 Date.prototype.getUTCFullYear()
    proto->DefineOwnProperty(
        this, Intern("getUTCFullYear"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetUTCFullYear, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.12 Date.prototype.getMonth()
    proto->DefineOwnProperty(
        this, Intern("getMonth"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetMonth, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.13 Date.prototype.getUTCMonth()
    proto->DefineOwnProperty(
        this, Intern("getUTCMonth"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetUTCMonth, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.14 Date.prototype.getDate()
    proto->DefineOwnProperty(
        this, Intern("getDate"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateGetDate, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.15 Date.prototype.getUTCDate()
    proto->DefineOwnProperty(
        this, Intern("getUTCDate"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateGetUTCDate, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.16 Date.prototype.getDay()
    proto->DefineOwnProperty(
        this, Intern("getDay"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateGetDay, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.17 Date.prototype.getUTCDay()
    proto->DefineOwnProperty(
        this, Intern("getUTCDay"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateGetUTCDay, 0),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.18 Date.prototype.getHours()
    proto->DefineOwnProperty(
        this, Intern("getHours"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetHours, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.19 Date.prototype.getUTCHours()
    proto->DefineOwnProperty(
        this, Intern("getUTCHours"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetUTCHours, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.20 Date.prototype.getMinutes()
    proto->DefineOwnProperty(
        this, Intern("getMinutes"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetMinutes, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.21 Date.prototype.getUTCMinutes()
    proto->DefineOwnProperty(
        this, Intern("getUTCMinutes"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetUTCMinutes, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.22 Date.prototype.getSeconds()
    proto->DefineOwnProperty(
        this, Intern("getSeconds"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetSeconds, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.23 Date.prototype.getUTCSeconds()
    proto->DefineOwnProperty(
        this, Intern("getUTCSeconds"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetUTCSeconds, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.24 Date.prototype.getMilliseconds()
    proto->DefineOwnProperty(
        this, Intern("getMilliseconds"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetMilliseconds, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.25 Date.prototype.getUTCMilliseconds()
    proto->DefineOwnProperty(
        this, Intern("getUTCMilliseconds"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetUTCMilliseconds, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.26 Date.prototype.getTimezoneOffset()
    proto->DefineOwnProperty(
        this, Intern("getTimezoneOffset"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateGetTimezoneOffset, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.27 Date.prototype.setTime(time)
    proto->DefineOwnProperty(
        this, Intern("setTime"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateSetTime, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.28 Date.prototype.setMilliseconds(ms)
    proto->DefineOwnProperty(
        this, Intern("setMilliseconds"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetMilliseconds, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.29 Date.prototype.setUTCMilliseconds(ms)
    proto->DefineOwnProperty(
        this, Intern("setUTCMilliseconds"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetUTCMilliseconds, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.30 Date.prototype.setSeconds(sec[, ms])
    proto->DefineOwnProperty(
        this, Intern("setSeconds"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetSeconds, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.31 Date.prototype.setUTCSeconds(sec[, ms])
    proto->DefineOwnProperty(
        this, Intern("setUTCSeconds"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetUTCSeconds, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.32 Date.prototype.setMinutes(min[, sec[, ms]])
    proto->DefineOwnProperty(
        this, Intern("setMinutes"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetMinutes, 3),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.33 Date.prototype.setUTCMinutes(min[, sec[, ms]])
    proto->DefineOwnProperty(
        this, Intern("setUTCMinutes"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetUTCMinutes, 3),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.34 Date.prototype.setHours(hour[, min[, sec[, ms]])
    proto->DefineOwnProperty(
        this, Intern("setHours"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetHours, 4),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.35 Date.prototype.setUTCHours(hour[, min[, sec[, ms]])
    proto->DefineOwnProperty(
        this, Intern("setUTCHours"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetUTCHours, 4),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.36 Date.prototype.setDate(date)
    proto->DefineOwnProperty(
        this, Intern("setDate"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateSetDate, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.37 Date.prototype.setUTCDate(date)
    proto->DefineOwnProperty(
        this, Intern("setUTCDate"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::DateSetUTCDate, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.38 Date.prototype.setMonth(month[, date])
    proto->DefineOwnProperty(
        this, Intern("setMonth"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetMonth, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.39 Date.prototype.setUTCMonth(month[, date])
    proto->DefineOwnProperty(
        this, Intern("setUTCMonth"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetUTCMonth, 2),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.40 Date.prototype.setFullYear(year[, month[, date]])
    proto->DefineOwnProperty(
        this, Intern("setFullYear"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetFullYear, 3),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.41 Date.prototype.setUTCFullYear(year[, month[, date]])
    proto->DefineOwnProperty(
        this, Intern("setUTCFullYear"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateSetUTCFullYear, 3),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.42 Date.prototype.toUTCString()
    proto->DefineOwnProperty(
        this, Intern("toUTCString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateToUTCString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.43 Date.prototype.toISOString()
    proto->DefineOwnProperty(
        this, Intern("toISOString"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateToISOString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.9.5.44 Date.prototype.toJSON()
    proto->DefineOwnProperty(
        this, Intern("toJSON"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::DateToJSON, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // RegExp
    JSObject* const proto = JSRegExp::NewPlain(this);
    // section 15.10.4 The RegExp Constructor
    JSNativeFunction* const constructor =
        JSNativeFunction::NewPlain(this, &runtime::RegExpConstructor, 2);
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
            JSNativeFunction::New(this, &runtime::RegExpExec, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.10.6.3 RegExp.prototype.test(string)
    proto->DefineOwnProperty(
        this, Intern("test"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::RegExpTest, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.10.6.4 RegExp.prototype.toString()
    proto->DefineOwnProperty(
        this, toString_symbol_,
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::RegExpToString, 0),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // section 15.12 JSON
    JSObject* const json = JSObject::NewPlain(this);
    json->set_prototype(obj_proto);
    const Symbol name = Intern("JSON");
    json->set_class_name(name);
    global_obj_.DefineOwnProperty(
        this, name,
        DataDescriptor(json,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.12.2 parse(text[, reviver])
    json->DefineOwnProperty(
        this, Intern("parse"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::JSONParse, 2),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.12.3 stringify(value[, replacer[, space]])
    json->DefineOwnProperty(
        this, Intern("stringify"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::JSONStringify, 3),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
  }

  {
    // Builtins
    // section 15.1.1.1 NaN
    global_obj_.DefineOwnProperty(
        this, Intern("NaN"),
        DataDescriptor(
            JSValData::kNaN, PropertyDescriptor::NONE),
        false, NULL);
    // section 15.1.1.2 Infinity
    global_obj_.DefineOwnProperty(
        this, Intern("Infinity"),
        DataDescriptor(
            std::numeric_limits<double>::infinity(), PropertyDescriptor::NONE),
        false, NULL);
    // section 15.1.1.3 undefined
    global_obj_.DefineOwnProperty(
        this, Intern("undefined"),
        DataDescriptor(JSUndefined, PropertyDescriptor::NONE),
        false, NULL);
    // section 15.1.2.1 eval(x)
    global_obj_.DefineOwnProperty(
        this, Intern("eval"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::GlobalEval, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    // section 15.1.2.3 parseIng(string, radix)
    global_obj_.DefineOwnProperty(
        this, Intern("parseInt"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::GlobalParseInt, 2),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    // section 15.1.2.3 parseFloat(string)
    global_obj_.DefineOwnProperty(
        this, Intern("parseFloat"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::GlobalParseFloat, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    // section 15.1.2.4 isNaN(number)
    global_obj_.DefineOwnProperty(
        this, Intern("isNaN"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::GlobalIsNaN, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);
    // section 15.1.2.5 isFinite(number)
    global_obj_.DefineOwnProperty(
        this, Intern("isFinite"),
        DataDescriptor(JSNativeFunction::New(this, &runtime::GlobalIsFinite, 1),
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // 15.1.3.1 decodeURI(encodedURI)
    global_obj_.DefineOwnProperty(
        this, Intern("decodeURI"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::GlobalDecodeURI, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // 15.1.3.2 decodeURIComponent(encodedURIComponent)
    global_obj_.DefineOwnProperty(
        this, Intern("decodeURIComponent"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::GlobalDecodeURIComponent, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.1.3.3 encodeURI(uri)
    global_obj_.DefineOwnProperty(
        this, Intern("encodeURI"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::GlobalEncodeURI, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    // section 15.1.3.4 encodeURIComponent(uriComponent)
    global_obj_.DefineOwnProperty(
        this, Intern("encodeURIComponent"),
        DataDescriptor(
            JSNativeFunction::New(this, &runtime::GlobalEncodeURIComponent, 1),
            PropertyDescriptor::WRITABLE |
            PropertyDescriptor::CONFIGURABLE),
        false, NULL);

    global_obj_.set_class_name(Intern("global"));
    global_obj_.set_prototype(obj_proto);
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
  throw_type_error_.Initialize(this, &runtime::ThrowTypeError, 0);
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
