#ifndef IV_LV5_BREAKER_ASSEMBLER_H_
#define IV_LV5_BREAKER_ASSEMBLER_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/noncopyable.h>
namespace iv {
namespace lv5 {
namespace breaker {

class Assembler : public Xbyak::CodeGenerator {
 public:
  class LocalLabelScope : core::Noncopyable<> {
   public:
    LocalLabelScope(Assembler* assembler)
      : assembler_(assembler) {
      assembler_->inLocalLabel();
    }
    ~LocalLabelScope() {
      assembler_->outLocalLabel();
    }
   private:
    Assembler* assembler_;
  };

  Assembler()
    : Xbyak::CodeGenerator(4096, Xbyak::AutoGrow) {
  }

  Executable GainExecutableByOffset(std::size_t offset) const {
    return reinterpret_cast<Executable>(getCode() + offset);
  }

  std::size_t size() const { return getSize(); }
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_ASSEMBLER_H_
