#ifndef _IV_LV5_JSON_STRINGIFIER_H_
#define _IV_LV5_JSON_STRINGIFIER_H_
#include <cassert>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include "uchar.h"
#include "noncopyable.h"
#include "conversions.h"
#include "ustring.h"
#include "lv5/lv5.h"
#include "lv5/property.h"
#include "lv5/jsval.h"
#include "lv5/jsarray.h"
#include "lv5/jsstring.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
#include "lv5/gc_template.h"
#include "lv5/context.h"
namespace iv {
namespace lv5 {
namespace detail {

class JSONStackScope : private core::Noncopyable<JSONStackScope>::type {
 public:
  JSONStackScope(trace::Vector<JSObject*>::type* stack, JSObject* obj)
    : stack_(stack) {
    stack_->push_back(obj);
  }
  ~JSONStackScope() {
    stack_->pop_back();
  }
 private:
  trace::Vector<JSObject*>::type* stack_;
};

static const char* kJSONNullStringPtr = "null";
static const core::UString kJSONNullString(kJSONNullStringPtr,
                                           kJSONNullStringPtr+4);

}  // namespace iv::lv5::detail

class JSONStringifier : private core::Noncopyable<JSONStringifier>::type {
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
    JSString* operator()(const Symbol& sym) {
      return ctx_->ToString(sym);
    }
   private:
    Context* ctx_;
  };

  JSString* Quote(const JSString& str, Error* e) {
    static const char kHexDigits[17] = "0123456789ABCDEF";
    StringBuilder builder;
    builder.Append('"');
    for (JSString::const_iterator it = str.begin(),
         last = str.end(); it != last; ++it) {
      const uc16 c = *it;
      if (c == '"' || c == '\\') {
        builder.Append('\\');
        builder.Append(c);
      } else if (c == '\b' ||
                 c == '\f' ||
                 c == '\n' ||
                 c == '\r' ||
                 c == '\t') {
        builder.Append('\\');
        switch (c) {
          case '\b':
            builder.Append('b');
            break;

          case '\f':
            builder.Append('f');
            break;

          case '\n':
            builder.Append('n');
            break;

          case '\r':
            builder.Append('r');
            break;

          case '\t':
            builder.Append('t');
            break;
        }
      } else if (c < ' ') {
        uint16_t val = c;
        std::tr1::array<char, 4> buf = { { '0', '0', '0', '0' } };
        builder.Append("\\u");
        for (int i = 0; (i < 4) || val; i++) {
          buf[3 - i] = kHexDigits[val % 16];
          val /= 16;
        }
        builder.Append(buf.data(), buf.size());
      } else {
        builder.Append(c);
      }
    }
    builder.Append('"');
    return builder.Build(ctx_);
  }

  void CyclicCheck(JSObject* value, Error* e) {
    using std::find;
    if (find(stack_.begin(), stack_.end(), value) != stack_.end()) {
      e->Report(Error::Type, "JSON.stringify not allow cyclical structure");
    }
  }

  JSVal JO(JSObject* value, Error* e) {
    CyclicCheck(value, ERROR(e));
    detail::JSONStackScope scope(&stack_, value);
    const core::UString stepback = indent_;
    indent_.append(gap_);

    trace::Vector<JSString*>::type prop;
    const trace::Vector<JSString*>::type* k;
    if (property_list_) {
      k = property_list_;
    } else {
      using std::transform;
      std::vector<Symbol> keys;
      value->GetOwnPropertyNames(ctx_, &keys, JSObject::kExcludeNotEnumerable);
      prop.resize(keys.size());
      transform(keys.begin(), keys.end(), prop.begin(), SymbolToString(ctx_));
      k = &prop;
    }

    std::vector<core::UString> partial;

    for (trace::Vector<JSString*>::type::const_iterator it = k->begin(),
         last = k->end(); it != last; ++it) {
      const JSVal result = Str(
          ctx_->Intern(core::UStringPiece((*it)->data(), (*it)->size())),
          value, ERROR(e));
      if (!result.IsUndefined()) {
        core::UString member;
        JSString* ret = Quote(**it, ERROR(e));
        member.append(ret->begin(), ret->end());
        member.push_back(':');
        if (!gap_.empty()) {
          member.push_back(' ');
        }
        assert(result.IsString());
        JSString* target = result.string();
        member.append(target->begin(), target->end());
        partial.push_back(member);
      }
    }

    JSString* final;
    if (partial.empty()) {
      final = JSString::NewAsciiString(ctx_, "{}");
    } else {
      StringBuilder builder;
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
        final = builder.Build(ctx_);
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
        final = builder.Build(ctx_);
      }
    }
    indent_.assign(stepback);
    return final;
  }

  JSVal JA(JSArray* value, Error* e) {
    CyclicCheck(value, ERROR(e));
    detail::JSONStackScope scope(&stack_, value);
    const core::UString stepback = indent_;
    indent_.append(gap_);

    std::vector<core::UString> partial;

    const JSVal length = value->Get(
        ctx_,
        ctx_->length_symbol(), ERROR(e));
    const double val = length.ToNumber(ctx_, ERROR(e));
    const uint32_t len = core::DoubleToUInt32(val);
    for (uint32_t index = 0; index < len; ++index) {
      JSVal str = Str(ctx_->InternIndex(index), value, ERROR(e));
      if (str.IsUndefined()) {
        partial.push_back(detail::kJSONNullString);
      } else {
        assert(str.IsString());
        const JSString* const s = str.string();
        partial.push_back(core::UString(s->begin(), s->end()));
      }
    }
    JSString* final;
    if (partial.empty()) {
      final = JSString::NewAsciiString(ctx_, "[]");
    } else {
      StringBuilder builder;
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
        final = builder.Build(ctx_);
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
        final = builder.Build(ctx_);
      }
    }
    indent_.assign(stepback);
    return final;
  }

  JSVal Str(Symbol key, JSObject* holder, Error* e) {
    JSVal value = holder->Get(ctx_, key, ERROR(e));
    if (value.IsObject()) {
      JSObject* const target = value.object();
      const JSVal method = target->Get(ctx_, ctx_->Intern("toJSON"), ERROR(e));
      if (method.IsCallable()) {
        Arguments args_list(ctx_, 1, ERROR(e));
        args_list[0] = ctx_->ToString(key);
        value = method.object()->AsCallable()->Call(args_list, target, ERROR(e));
      }
    }
    if (replacer_) {
      Arguments args_list(ctx_, 2, ERROR(e));
      args_list[0] = ctx_->ToString(key);
      args_list[1] = value;
      value = replacer_->Call(args_list, holder, ERROR(e));
    }
    if (value.IsObject()) {
      JSObject* const target = value.object();
      if (target->class_name() == ctx_->Intern("Number")) {
        value = value.ToNumber(ctx_, ERROR(e));
      } else if (target->class_name() == ctx_->Intern("String")) {
        value = value.ToString(ctx_, ERROR(e));
      } else if (target->class_name() == ctx_->Intern("Boolean")) {
        value = JSVal::Bool(static_cast<JSBooleanObject*>(target)->value());
      }
    }
    if (value.IsNull()) {
      return JSString::NewAsciiString(ctx_, "null");
    }
    if (value.IsBoolean()) {
      if (value.boolean()) {
        return JSString::NewAsciiString(ctx_, "true");
      } else {
        return JSString::NewAsciiString(ctx_, "false");
      }
    }
    if (value.IsString()) {
      return Quote(*(value.string()), e);
    }
    if (value.IsNumber()) {
      const double val = value.number();
      if (std::isfinite(val)) {
        return value.ToString(ctx_, e);
      } else {
        return JSString::NewAsciiString(ctx_, "null");
      }
    }
    if (value.IsObject() && !value.IsCallable()) {
      JSObject* target = value.object();
      if (ctx_->IsArray(*target)) {
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
#endif  // _IV_LV5_JSON_STRINGIFIER_H_
