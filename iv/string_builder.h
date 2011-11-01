#ifndef IV_STRING_BUILDER_H_
#define IV_STRING_BUILDER_H_
#include <vector>
#include <iterator>
#include <string>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
namespace iv {
namespace core {

template<typename CharT>
class BasicStringBuilder : protected std::vector<CharT> {
 public:
  typedef BasicStringBuilder this_type;
  typedef std::vector<CharT> container_type;

  typedef typename container_type::value_type value_type;
  typedef typename container_type::pointer pointer;
  typedef typename container_type::const_pointer const_pointer;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::const_iterator const_iterator;
  typedef typename container_type::reference reference;
  typedef typename container_type::const_reference const_reference;
  typedef typename container_type::reverse_iterator reverse_iterator;
  typedef typename container_type::const_reverse_iterator const_reverse_iterator;
  typedef typename container_type::size_type size_type;
  typedef typename container_type::difference_type difference_type;

  void Append(const core::UStringPiece& piece) {
    insert(container_type::end(), piece.begin(), piece.end());
  }

  void Append(const core::StringPiece& piece) {
    insert(container_type::end(), piece.begin(), piece.end());
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

  using container_type::push_back;

  using container_type::insert;

  using container_type::assign;

  using container_type::clear;

  core::BasicStringPiece<CharT> BuildPiece() const {
    return core::BasicStringPiece<CharT>(container_type::data(),
                                         container_type::size());
  }

  std::basic_string<CharT> Build() const {
    return std::basic_string<CharT>(container_type::data(),
                                    container_type::size());
  }
};

typedef BasicStringBuilder<char> StringBuilder;
typedef BasicStringBuilder<uint16_t> UStringBuilder;

} }  // namespace iv::core
#endif  // IV_STRING_BUILDER_H_
