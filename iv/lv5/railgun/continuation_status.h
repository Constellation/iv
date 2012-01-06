#ifndef IV_LV5_RAILGUN_CONTINUATION_STATUS_H_
#define IV_LV5_RAILGUN_CONTINUATION_STATUS_H_
#include <iv/detail/unordered_set.h>
#include <iv/noncopyable.h>
#include <iv/lv5/specialized_ast.h>
namespace iv {
namespace lv5 {
namespace railgun {
namespace continuation_status_detail {

const Statement* kNextStatement = NULL;

}  // namespace continuation_status_detail

// save continuation status and find dead code
class ContinuationStatus : private core::Noncopyable<ContinuationStatus> {
 public:
  typedef std::unordered_set<const Statement*> ContinuationSet;

  ContinuationStatus()
    : current_() {
    Insert(continuation_status_detail::kNextStatement);
  }

  void Insert(const Statement* stmt) {
    current_.insert(stmt);
  }

  void Erase(const Statement* stmt) {
    current_.erase(stmt);
  }

  void Kill() {
    Erase(continuation_status_detail::kNextStatement);
  }

  bool Has(const Statement* stmt) const {
    return current_.count(stmt) != 0;
  }

  bool IsDeadStatement() const {
    return !Has(continuation_status_detail::kNextStatement);
  }

  void JumpTo(const BreakableStatement* target) {
    Kill();
    Insert(target);
  }

  void ResolveJump(const BreakableStatement* target) {
    if (Has(target)) {
      Erase(target);
      Insert(continuation_status_detail::kNextStatement);
    }
  }

  void Clear() {
    current_.clear();
    Insert(continuation_status_detail::kNextStatement);
  }

  void Next() {
    Insert(continuation_status_detail::kNextStatement);
  }

 private:
  ContinuationSet current_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CONTINUATION_STATUS_H_
