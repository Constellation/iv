#ifndef _IV_LV5_STRING_BUILDER_H_
#define _IV_LV5_STRING_BUILDER_H_
#include <vector>
#include <iterator>
#include "lv5/jsstring.h"
namespace iv {
namespace lv5 {

template<typename CharT = uint16_t>
class BasicStringBuilder : protected std::vector<CharT> {
 public:
  typedef BasicStringBuilder this_type;
  typedef std::vector<CharT> container_type;

  void Append(const core::UStringPiece& piece) {
    insert(container_type::end(), piece.begin(), piece.end());
  }

  void Append(const core::StringPiece& piece) {
    insert(container_type::end(), piece.begin(), piece.end());
  }

  void Append(const JSString& str) {
    str.Copy(std::back_inserter<container_type>(*this));
  }

  void Append(CharT ch) {
    push_back(ch);
  }

  template<typename Iter>
  void Append(Iter it, typename container_type::size_type size) {
    insert(container_type::end(), it, it + size);
  }

  template<typename Iter>
  void Append(Iter start, Iter last) {
    insert(container_type::end(), start, last);
  }

  // for assignable object (like std::string)

  void append(const core::UStringPiece& piece) {
    insert(container_type::end(), piece.begin(), piece.end());
  }

  void append(const core::StringPiece& piece) {
    insert(container_type::end(), piece.begin(), piece.end());
  }

  void append(const JSString& str) {
    str.Copy(std::back_inserter<container_type>(*this));
  }

  using container_type::push_back;

  using container_type::insert;

  using container_type::assign;

  JSString* Build(Context* ctx) const {
    return JSString::New(ctx, container_type::begin(), container_type::end());
  }

  core::UStringPiece BuildUStringPiece() const {
    return core::UStringPiece(container_type::data(), container_type::size());
  }

  core::UString BuildUString() const {
    return core::UString(container_type::data(), container_type::size());
  }
};

typedef BasicStringBuilder<uint16_t> StringBuilder;

} }  // namespace iv::lv5
#endif  // _IV_LV5_STRING_BUILDER_H_
