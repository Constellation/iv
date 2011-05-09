#ifndef _IV_LV5_EVAL_SOURCE_H_
#define _IV_LV5_EVAL_SOURCE_H_
#include "noncopyable.h"
#include "source_traits.h"
#include "lv5/jsstring.h"
namespace iv {
namespace lv5 {
namespace detail {

static const std::string kEvalSource = "(eval)";

}  // namespace detail

class EvalSource : public core::Noncopyable<> {
 public:
  EvalSource(const JSString& str)
    : source_(str) {
  }

  inline uc16 operator[](std::size_t pos) const {
    assert(pos < size());
    return source_[pos];
  }

  inline std::size_t size() const {
    return source_.size();
  }

  inline core::UStringPiece SubString(
      std::size_t n, std::size_t len = std::string::npos) const {
    if (len == std::string::npos) {
      return core::UStringPiece((source_.data() + n), (source_.size() - n));
    } else {
      return core::UStringPiece((source_.data() + n), len);
    }
  }
 private:
  const JSString& source_;
};

}  // namespace lv5
namespace core {

template<>
struct SourceTraits<lv5::EvalSource> {
  static std::string GetFileName(const lv5::EvalSource& src) {
    return lv5::detail::kEvalSource;
  }
};

} }  // namespace iv::core
#endif  // _IV_LV5_EVAL_SOURCE_H_
