#ifndef IV_COUNTER_H_
#define IV_COUNTER_H_
namespace iv {
namespace core {

template<typename Derived>
class Counter {
 public:
  Counter(uint64_t initial = 0) : counter_(initial) { }
  void operator()() {
    *(static_cast<Derived*>(this))(counter_++);
  }
 private:
  uint64_t counter_;
};

} }  // namespace iv::core
#endif  // IV_COUNTER_H_
