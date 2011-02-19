#ifndef _IV_LV5_EVAL_SOURCE_H_
#define _IV_LV5_EVAL_SOURCE_H_
#include "noncopyable.h"
#include "lv5/jsstring.h"
namespace iv {
namespace lv5 {
namespace detail {

static const std::string kEvalSource = "(eval)";

}  // namespace iv::lv5::detail

class EvalSource : public core::Noncopyable<EvalSource>::type {
 public:
  EvalSource(const JSString& str)
    : source_(str) {
  }

  inline uc16 Get(std::size_t pos) const {
    assert(pos < size());
    return source_[pos];
  }

  inline std::size_t size() const {
    return source_.size();
  }

  inline const std::string& filename() const {
    return detail::kEvalSource;
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

} }  // namespace iv::lv5
#endif  // _IV_LV5_EVAL_SOURCE_H_
