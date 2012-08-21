#ifndef IV_LV5_JSSTRING_BUILDER_H_
#define IV_LV5_JSSTRING_BUILDER_H_
#include <iv/string_builder.h>
#include <iv/lv5/jsstring_fwd.h>
namespace iv {
namespace lv5 {

class JSStringBuilder : public core::BasicStringBuilder<uint16_t> {
 public:
  JSString* Build(Context* ctx, bool is_8bit, Error* e) const {
    return JSString::New(ctx,
                         container_type::begin(),
                         container_type::end(),
                         is_8bit, e);
  }

  // override Append is failed...
  void AppendJSString(const JSString& str) {
    const size_t current_size = container_type::size();
    container_type::resize(current_size + str.size());
    str.Copy(container_type::begin() + current_size);
  }

  void AppendJSString(const JSString& str,
                      size_t from,
                      size_t to) {
    const size_t current_size = container_type::size();
    container_type::resize(current_size + (to - from));
    str.Copy(from, to, container_type::begin() + current_size);
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSSTRING_BUILDER_H_
