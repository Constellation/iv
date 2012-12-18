#ifndef IV_LV5_JSON_STRINGIFIER_H_
#define IV_LV5_JSON_STRINGIFIER_H_
#include <cassert>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <iv/platform_math.h>
#include <iv/noncopyable.h>
#include <iv/conversions.h>
#include <iv/ustring.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/property.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jsnumberobject.h>
#include <iv/lv5/jsstring_builder.h>
#include <iv/lv5/jsbooleanobject.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/context.h>
namespace iv {
namespace lv5 {
namespace detail {

class JSONStackScope : private core::Noncopyable<> {
 public:
  static const std::size_t kJSONMaxRecursion = 4096;
  JSONStackScope(trace::Vector<JSObject*>::type* stack, JSObject* obj, Error* e)
    : stack_(stack) {
    // cyclic check
    if (std::find(stack_->begin(), stack_->end(), obj) != stack_->end()) {
      e->Report(Error::Type, "JSON.stringify not allow cyclical structure");
      stack_ = NULL;
    } else {
      stack_->push_back(obj);
      // stack depth check
      if (stack_->size() > kJSONMaxRecursion) {
        e->Report(Error::Range, "max stack exceeded in JSON.stringify");
      }
    }
  }
  ~JSONStackScope() {
    if (stack_) {
      stack_->pop_back();
    }
  }
 private:
  trace::Vector<JSObject*>::type* stack_;
};

static const core::UString kJSONNullString = core::ToUString("null");

}  // namespace detail

class JSONStringifier : private core::Noncopyable<> {
 public:
  JSONStringifier(Context* ctx,
                  JSFunction* replacer,
                  const core::UString& gap,
                  const trace::Vector<JSString*>::type* property_list)
    : ctx_(ctx),
      replacer_(replacer),
      stack_(),
      indent_(),
      gap_(gap),
      property_list_(property_list) {
  }

  JSVal Stringify(Symbol key, JSObject* holder, Error* e) {
    return Str(key, holder, e);
  }

 private:

  class SymbolToString {
   public:
    explicit SymbolToString(Context* ctx) : ctx_(ctx) { }
    JSString* operator()(const Symbol& key) {
      return JSString::New(ctx_, key);
    }
   private:
    Context* ctx_;
  };

  JSString* Quote(const JSString& str, Error* e) {
    JSStringBuilder builder;
    builder.Append('"');
    if (str.Is8Bit()) {
      const Fiber8* fiber = str.Get8Bit();
      core::JSONQuote(fiber->begin(),
                      fiber->end(), std::back_inserter(builder));
    } else {
      const Fiber16* fiber = str.Get16Bit();
      core::JSONQuote(fiber->begin(),
                      fiber->end(), std::back_inserter(builder));
    }
    builder.Append('"');
    return builder.Build(ctx_, str.Is8Bit(), e);
  }

  JSVal JO(JSObject* value, Error* e) {
    detail::JSONStackScope scope(&stack_, value, IV_LV5_ERROR(e));
    const core::UString stepback = indent_;
    indent_.append(gap_);

    trace::Vector<JSString*>::type prop;
    const trace::Vector<JSString*>::type* k;
    if (property_list_) {
      k = property_list_;
    } else {
      PropertyNamesCollector collector;
      value->GetOwnPropertyNames(ctx_, &collector, EXCLUDE_NOT_ENUMERABLE);
      prop.resize(collector.names().size());
      std::transform(collector.names().begin(), collector.names().end(),
                     prop.begin(), SymbolToString(ctx_));
      k = &prop;
    }

    std::vector<core::UString> partial;

    for (trace::Vector<JSString*>::type::const_iterator it = k->begin(),
         last = k->end(); it != last; ++it) {
      const JSVal result = Str(ctx_->Intern(*it), value, IV_LV5_ERROR(e));
      if (!result.IsUndefined()) {
        core::UString member;
        JSString* ret = Quote(**it, IV_LV5_ERROR(e));
        ret->Copy(std::back_inserter(member));
        member.push_back(':');
        if (!gap_.empty()) {
          member.push_back(' ');
        }
        assert(result.IsString());
        JSString* target = result.string();
        target->Copy(std::back_inserter(member));
        partial.push_back(member);
      }
    }

    JSString* final;
    if (partial.empty()) {
      // no error
      final = JSString::NewAsciiString(ctx_, "{}", e);
    } else {
      JSStringBuilder builder;
      if (gap_.empty()) {
        builder.Append('{');
        std::vector<core::UString>::const_iterator
            it = partial.begin(), last = partial.end();
        while (it != last) {
          builder.Append(*it);
          ++it;
          if (it == last) {
            break;
          } else {
            builder.Append(',');
          }
        }
        builder.Append('}');
        final = builder.Build(ctx_, false, IV_LV5_ERROR(e));
      } else {
        builder.Append("{\n");
        builder.Append(indent_);
        std::vector<core::UString>::const_iterator
            it = partial.begin(), last = partial.end();
        while (it != last) {
          builder.Append(*it);
          ++it;
          if (it == last) {
            break;
          } else {
            builder.Append(",\n");
            builder.Append(indent_);
          }
        }
        builder.Append('\n');
        builder.Append(stepback);
        builder.Append('}');
        final = builder.Build(ctx_, false, IV_LV5_ERROR(e));
      }
    }
    indent_.assign(stepback);
    return final;
  }

  JSVal JA(JSArray* value, Error* e) {
    detail::JSONStackScope scope(&stack_, value, IV_LV5_ERROR(e));
    const core::UString stepback = indent_;
    indent_.append(gap_);

    std::vector<core::UString> partial;

    const uint32_t len = internal::GetLength(ctx_, value, IV_LV5_ERROR(e));
    for (uint32_t index = 0; index < len; ++index) {
      JSVal str = Str(ctx_->Intern(index), value, IV_LV5_ERROR(e));
      if (str.IsUndefined()) {
        partial.push_back(detail::kJSONNullString);
      } else {
        assert(str.IsString());
        const JSString* const s = str.string();
        partial.push_back(s->GetUString());
      }
    }
    JSString* final;
    if (partial.empty()) {
      // no error
      final = JSString::NewAsciiString(ctx_, "[]", e);
    } else {
      JSStringBuilder builder;
      if (gap_.empty()) {
        builder.Append('[');
        std::vector<core::UString>::const_iterator
            it = partial.begin(), last = partial.end();
        while (it != last) {
          builder.Append(*it);
          ++it;
          if (it == last) {
            break;
          } else {
            builder.Append(',');
          }
        }
        builder.Append(']');
        final = builder.Build(ctx_, false, IV_LV5_ERROR(e));
      } else {
        builder.Append("[\n");
        builder.Append(indent_);
        std::vector<core::UString>::const_iterator
            it = partial.begin(), last = partial.end();
        while (it != last) {
          builder.Append(*it);
          ++it;
          if (it == last) {
            break;
          } else {
            builder.Append(",\n");
            builder.Append(indent_);
          }
        }
        builder.Append('\n');
        builder.Append(stepback);
        builder.Append(']');
        final = builder.Build(ctx_, false, IV_LV5_ERROR(e));
      }
    }
    indent_.assign(stepback);
    return final;
  }

  JSVal Str(Symbol key, JSObject* holder, Error* e) {
    JSVal value = holder->Get(ctx_, key, IV_LV5_ERROR(e));
    if (value.IsObject()) {
      JSObject* const target = value.object();
      const JSVal method = target->Get(ctx_, symbol::toJSON(), IV_LV5_ERROR(e));
      if (method.IsCallable()) {
        ScopedArguments args_list(ctx_, 1, IV_LV5_ERROR(e));
        args_list[0] = JSString::New(ctx_, key);
        value =
           static_cast<JSFunction*>(
               method.object())->Call(&args_list, target, IV_LV5_ERROR(e));
      }
    }
    if (replacer_) {
      ScopedArguments args_list(ctx_, 2, IV_LV5_ERROR(e));
      args_list[0] = JSString::New(ctx_, key);
      args_list[1] = value;
      value = replacer_->Call(&args_list, holder, IV_LV5_ERROR(e));
    }
    if (value.IsObject()) {
      JSObject* const target = value.object();
      if (target->IsClass<Class::Number>()) {
        value = value.ToNumber(ctx_, IV_LV5_ERROR(e));
      } else if (target->IsClass<Class::String>()) {
        value = value.ToString(ctx_, IV_LV5_ERROR(e));
      } else if (target->IsClass<Class::Boolean>()) {
        value = JSVal::Bool(static_cast<JSBooleanObject*>(target)->value());
      }
    }
    if (value.IsNull()) {
      return ctx_->global_data()->string_null();
    }
    if (value.IsBoolean()) {
      if (value.boolean()) {
        return ctx_->global_data()->string_true();
      } else {
        return ctx_->global_data()->string_false();
      }
    }
    if (value.IsString()) {
      return Quote(*(value.string()), e);
    }
    if (value.IsNumber()) {
      const double val = value.number();
      if (core::math::IsFinite(val)) {
        return value.ToString(ctx_, e);
      } else {
        return ctx_->global_data()->string_null();
      }
    }
    if (value.IsObject() && !value.IsCallable()) {
      JSObject* target = value.object();
      if (target->IsClass<Class::Array>()) {
        return JA(static_cast<JSArray*>(target), e);
      } else {
        return JO(target, e);
      }
    }
    return JSUndefined;
  }

  Context* ctx_;
  JSFunction* replacer_;
  trace::Vector<JSObject*>::type stack_;
  core::UString indent_;
  core::UString gap_;
  const trace::Vector<JSString*>::type* property_list_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSON_STRINGIFIER_H_
