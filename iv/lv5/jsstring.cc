#include <iv/unicode.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jsarray.h>
#include <iv/lv5/jsvector.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsstring_builder.h>
namespace iv {
namespace lv5 {

JSString::JSString(Context* ctx, int32_t size)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr)
  , size_(size)
  , flags_(0)
  , data_() {
}

JSString::JSString(Context* ctx)
  : JSCell(radio::STRING, ctx->global_data()->primitive_string_map(), nullptr)
  , size_(0)
  , flags_(STRING_SEQ | IS_8BIT)
  , data_() {
}

JSString* JSString::New(Context* ctx, char16_t ch) {
  if (JSString* res = ctx->global_data()->GetSingleString(ch)) {
    return res;
  }
  // TODO(Yusuke Suzuki):
  // Show this error.
  Error::Dummy dummy;
  if (core::character::IsASCII(ch)) {
    const char ascii = ch;
    return new JSSeqString(ctx, true, &ascii, &ascii + 1);
  } else {
    const char16_t utf16 = ch;
    return new JSSeqString(ctx, false, &utf16, &utf16 + 1);
  }
}

JSString* JSString::NewEmpty(Context* ctx) {
  return ctx->global_data()->string_empty();
}

// TODO(Yusuke Suzuki):
// If it is JSConsString, we should attempt to extract string as cons.
// And we should remove this const_cast.
JSString* JSString::Substring(Context* ctx,
                              size_type from,
                              size_type to) const {
  return JSSlicedString::New(
      ctx,
      const_cast<JSString*>(this),
      from, to == npos ? size() : to);
}

JSString::size_type JSString::find(const JSString& target,
                                   size_type index) const {
  if (Is8Bit() == target.Is8Bit()) {
    // same type
    if (Is8Bit()) {
      return view8().find(target.view8(), index);
    } else {
      return view16().find(target.view16(), index);
    }
  }
  if (Is8Bit()) {
    const core::u16string_view rhs = target.view16();
    return view8().find(rhs.begin(), rhs.end(), index);
  } else {
    const core::string_view rhs = target.view8();
    return view16().find(rhs.begin(), rhs.end(), index);
  }
}

JSString::size_type JSString::rfind(const JSString& target,
                                    size_type index) const {
  if (Is8Bit() == target.Is8Bit()) {
    // same type
    if (Is8Bit()) {
      return view8().rfind(target.view8(), index);
    } else {
      return view16().rfind(target.view16(), index);
    }
  }
  if (Is8Bit()) {
    const core::u16string_view rhs = target.view16();
    return view8().rfind(rhs.begin(), rhs.end(), index);
  } else {
    const core::string_view rhs = target.view8();
    return view16().rfind(rhs.begin(), rhs.end(), index);
  }
}

std::string JSString::GetUTF8() const {
  if (Is8Bit()) {
    return view8();
  }
  const core::u16string_view view = view16();
  std::string str;
  str.reserve(size() * 2);
  if (core::unicode::UTF16ToUTF8(
          view.begin(),
          view.end(),
          std::back_inserter(str)) != core::unicode::UNICODE_NO_ERROR) {
    str.clear();
  }
  return str;
}

std::u16string JSString::GetUTF16() const {
  if (Is16Bit()) {
    return view16();
  }
  const core::string_view view = view8();
  std::u16string str;
  str.resize(size());
  std::copy(view.begin(), view.end(), str.begin());
  return str;
}

JSString* JSString::Repeat(Context* ctx, uint32_t count, Error* e) {
  if (count == 0 || empty()) {
    return NewEmpty(ctx);
  }
  if (count == 1) {
    return this;
  }
  JSString* current = this;
  JSString* result = NewEmpty(ctx);
  do {
    if (count & 1) {
      result = JSString::NewCons(ctx, result, current, IV_LV5_ERROR(e));
    }
    count >>= 1;
    current = JSString::NewCons(ctx, current, current, IV_LV5_ERROR(e));
  } while (count > 0);
  return result;
}

std::ostream& operator<<(std::ostream& os, const JSString& str) {
  if (str.Is8Bit()) {
    auto view = str.view8();
    os.write(view.data(), view.size());
  } else {
    auto view = str.view16();
    core::unicode::UTF16ToUTF8(view.begin(),
                               view.end(),
                               std::ostream_iterator<char>(os));
  }
  return os;
}

char16_t JSFlatString::operator[](size_type n) const {
  if (Is8Bit()) {
    return (*As<char>())[n];
  } else {
    return (*As<char16_t>())[n];
  }
}

int JSFlatString::compare(const this_type& x) const {
  if (Is8Bit() == x.Is8Bit()) {
    // same type. use downcast
    if (Is8Bit()) {
      return As<char>()->compare(*x.As<char>());
    } else {
      return As<char16_t>()->compare(*x.As<char16_t>());
    }
  }
  if (Is8Bit()) {
    return core::CompareIterators(
        As<char>()->begin(), As<char>()->end(),
        x.As<char16_t>()->begin(), x.As<char16_t>()->end());
  } else {
    return core::CompareIterators(
        As<char16_t>()->begin(), As<char16_t>()->end(),
        x.As<char>()->begin(), x.As<char>()->end());
  }
}

char16_t JSString::At(size_type n) const {
  assert(n < size());
  if (IsFlat()) {
    return (*static_cast<const JSFlatString*>(this))[n];
  }
  Flatten();
  return (*static_cast<const JSFlatString*>(this))[n];
}

JSString* JSString::New(Context* ctx, Symbol sym) {
  if (symbol::IsIndexSymbol(sym)) {
    const uint32_t index = symbol::GetIndexFromSymbol(sym);
    if (index < 10) {
      return New(ctx, index + '0');
    }
    std::array<char, 15> buffer;
    char* end = core::UInt32ToString(index, buffer.data());
    Error::Dummy dummy;
    return JSSeqString::New(ctx, buffer.data(), end, true, &dummy);
  } else {
    assert(!symbol::IsIndexSymbol(sym));
    const std::u16string* str = symbol::GetStringFromSymbol(sym);
    if (str->empty()) {
      return NewEmpty(ctx);
    }
    if (str->size() == 1) {
      return New(ctx, (*str)[0]);
    }
    assert(str->size() <= kMaxSize);
    return JSString::NewExternal(ctx, *str);
  }
}

JSString* JSString::NewCons(Context* ctx,
                            JSVal* src,
                            uint32_t count,
                            Error* e) {
  if (count == 0) {
    return NewEmpty(ctx);
  }
  if (count == 1) {
    return src[0].string();
  }
  JSString* cons = JSString::NewCons(ctx,
                                     src[0].string(),
                                     src[1].string(),
                                     IV_LV5_ERROR(e));
  for (uint32_t i = 2; i < count; ++i) {
    cons = JSString::NewCons(ctx, cons, src[i].string(), IV_LV5_ERROR(e));
  }
  return cons;
}

template<typename CharIterator, typename OutputIterator>
inline OutputIterator SplitString(Context* ctx,
                                  CharIterator i,
                                  CharIterator iz,
                                  OutputIterator j) {
  for (; i != iz; ++i, ++j) {
    *j = JSString::New(ctx, *i);
  }
  return j;
}

// "STRING".split("") => ['S', 'T', 'R', 'I', 'N', 'G']
JSArray* JSString::Split(Context* ctx, uint32_t limit, Error* e) const {
  Flatten();
  const uint32_t smaller = (std::min<uint32_t>)(limit, size());
  JSVector* vec = JSVector::New(ctx, smaller);
  if (Is8Bit()) {
    const core::string_view view = view8();
    SplitString(ctx, view.begin(), view.begin() + smaller, vec->begin());
  } else {
    const core::u16string_view view = view16();
    SplitString(ctx, view.begin(), view.begin() + smaller, vec->begin());
  }
  return vec->ToJSArray();
}

JSArray* JSString::Split(Context* ctx,
                         char16_t ch,
                         uint32_t limit,
                         Error* e) const {
  JSVector* vec = JSVector::New(ctx);
  Flatten();
  if (Is8Bit()) {
    if (!core::character::IsASCII(ch)) {
      if (limit != 0) {
        // TODO(Yusuke Suzuki):
        // Remove const_cast.
        vec->push_back(const_cast<JSString*>(this));
      }
      return vec->ToJSArray();
    }
    const core::string_view view = view8();
    std::size_t index = 0;
    for (uint32_t i = 0; i < limit; ++i) {
      const std::size_t pos = view.find(ch, index);
      if (pos == core::string_view::npos) {
        vec->push_back(Substring(ctx, index, size()));
        break;
      }
      vec->push_back(Substring(ctx, index, pos));
      index = pos + 1;
    }
  } else {
    const core::u16string_view view = view16();
    std::size_t index = 0;
    for (uint32_t i = 0; i < limit; ++i) {
      const std::size_t pos = view.find(ch, index);
      if (pos == core::u16string_view::npos) {
        vec->push_back(Substring(ctx, index, size()));
        break;
      }
      vec->push_back(Substring(ctx, index, pos));
      index = pos + 1;
    }
  }
  return vec->ToJSArray();
}

} }  // namespace iv::lv5
