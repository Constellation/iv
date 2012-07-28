#ifndef IV_BREAKER_IC_H_
#define IV_BREAKER_IC_H_
namespace iv {
namespace lv5 {
namespace breaker {

class IC {
 public:
  enum Type {
    MONO,
    POLY
  };

  explicit IC(Type type)
    : type_(type) {
  }

 private:
  Type type_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_MONO_IC_H_
