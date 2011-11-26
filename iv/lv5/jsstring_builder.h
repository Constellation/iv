#ifndef IV_LV5_JSSTRING_BUILDER_H_
#define IV_LV5_JSSTRING_BUILDER_H_
#include <iv/string_builder.h>
#include <iv/lv5/jsstring_fwd.h>
namespace iv {
namespace lv5 {

class JSStringBuilder : public core::BasicStringBuilder<uint16_t> {
 public:
  JSString* Build(Context* ctx, bool is_8bit = false) const {
    return JSString::New(ctx,
                         container_type::begin(),
                         container_type::end(),
                         is_8bit);
  }

  // override Append is failed...
  void AppendJSString(const JSString& str) {
    str.Copy(std::back_inserter<container_type>(*this));
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRING_BUILDER_H_
