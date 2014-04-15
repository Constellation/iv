#ifndef IV_AERO_QUICK_CHECK_FWD_H_
#define IV_AERO_QUICK_CHECK_FWD_H_
#include <cassert>
#include <iv/noncopyable.h>
#include <iv/bloom_filter.h>
#include <iv/small_vector.h>
#include <iv/aero/visitor.h>
namespace iv {
namespace aero {

class Compiler;

class QuickCheck : private Visitor {
 public:
  class Aggregator {
   public:
    typedef core::BloomFilter<uint16_t> filter_type;
    typedef core::small_vector<char16_t, 8> container_type;

    Aggregator()
        : filter_()
        , characters_()
        , give_up_(false) { }

    bool Add(char16_t ch) {
      if (is_give_up()) {
        return false;
      }

      // Already added?
      if (std::any_of(
              characters_.begin(),
              characters_.end(),
              [ch](char16_t v) -> bool { return v == ch; })) {
        return true;
      }

      if (characters_.size() >= characters_.static_size()) {
        give_up_ = true;
        return false;
      }

      characters_.push_back(ch);
      filter_.Add(ch);
      return true;
    }

    void GiveUp() {
      give_up_ = true;
    }

    const filter_type& filter() const { return filter_; }

    const container_type& characters() const { return characters_; }

    bool is_give_up() const { return give_up_; }

    std::size_t count() const { return characters_.size(); }

   private:
    filter_type filter_;
    container_type characters_;
    bool give_up_;
  };

  class FilterCheck : core::Noncopyable<FilterCheck> {
   public:
    FilterCheck(QuickCheck* check) : check_(check) { }
    ~FilterCheck() {
      if (check_->aggregator().filter().Contains(0xFF) ||
          check_->aggregator().filter().Contains(0xFFFF)) {
        check_->Fail();
      }
    }
   private:
    QuickCheck* check_;
  };

  explicit QuickCheck(Compiler* compiler)
    : compiler_(compiler),
      aggregator_(),
      enabled_(true) { }

  std::pair<uint16_t, std::size_t> Emit(Disjunction* dis);

  void Fail() {
    aggregator_.GiveUp();
  }

  bool IsFailed() const {
    return aggregator_.is_give_up();
  }

  const Aggregator& aggregator() const { return aggregator_; }

 private:
  void Visit(Disjunction* dis);
  void Visit(Alternative* alt);
  void Visit(HatAssertion* assertion);
  void Visit(DollarAssertion* assertion);
  void Visit(EscapedAssertion* assertion);
  void Visit(DisjunctionAssertion* assertion);
  void Visit(BackReferenceAtom* atom);
  void Visit(CharacterAtom* atom);
  void Visit(StringAtom* atom);
  void Visit(RangeAtom* atom);
  void Visit(DisjunctionAtom* atom);
  void Visit(Quantifiered* atom);

  void Emit(uint16_t ch) {
    aggregator_.Add(ch);
  }

  Compiler* compiler_;
  Aggregator aggregator_;
  bool enabled_;
};

} }  // namespace iv::aero
#endif  // IV_AERO_QUICK_CHECK_FWD_H_
