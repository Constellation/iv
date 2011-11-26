#ifndef IV_AERO_QUICK_CHECK_FWD_H_
#define IV_AERO_QUICK_CHECK_FWD_H_
#include <iv/noncopyable.h>
#include <iv/bloom_filter.h>
#include <iv/aero/visitor.h>
namespace iv {
namespace aero {

class Compiler;

class QuickCheck : private Visitor {
 public:
  class FilterCheck : core::Noncopyable<FilterCheck> {
   public:
    FilterCheck(QuickCheck* check) : check_(check) { }
    ~FilterCheck() {
      if (check_->filter().Contains(0xFF) ||
          check_->filter().Contains(0xFFFF)) {
        check_->Fail();
      }
    }
   private:
    QuickCheck* check_;
  };
  explicit QuickCheck(Compiler* compiler)
    : compiler_(compiler),
      filter_(),
      enabled_(true),
      added_character_count_(0) { }

  std::pair<uint16_t, std::size_t> Emit(Disjunction* dis);

  const core::BloomFilter<uint16_t>& filter() {
    return filter_;
  }

  std::size_t added_character_count() const {
    return added_character_count_;
  }

  void Fail() {
    enabled_ = false;
  }

  bool IsFailed() const {
    return !enabled_;
  }
 private:
  void Visit(Disjunction* dis);
  void Visit(Alternative* alt);
  void Visit(HatAssertion* assertion);
  void Visit(DollarAssertion* assertion);
  void Visit(EscapedAssertion* assertion);
  void Visit(DisjunctionAssertion* assertion);
  void Visit(BackReferenceAtom* atom);
  void Visit(CharacterAtom* atom);
  void Visit(RangeAtom* atom);
  void Visit(DisjunctionAtom* atom);
  void Visit(Quantifiered* atom);

  void Emit(uint16_t ch) {
    filter_.Add(ch);
    ++added_character_count_;
  }

  Compiler* compiler_;
  core::BloomFilter<uint16_t> filter_;
  bool enabled_;
  std::size_t added_character_count_;
};

} }  // namespace iv::aero
#endif  // IV_AERO_QUICK_CHECK_FWD_H_
