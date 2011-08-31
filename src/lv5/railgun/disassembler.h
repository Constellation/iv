#ifndef IV_LV5_RAILGUN_DISASSEMBLER_H_
#define IV_LV5_RAILGUN_DISASSEMBLER_H_
#include <vector>
#include <sstream>
#include "detail/array.h"
#include "detail/unordered_map.h"
#include "noncopyable.h"
#include "stringpiece.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/code.h"
#include "lv5/railgun/direct_threading.h"
namespace iv {
namespace lv5 {
namespace railgun {

template<typename Derived>
class DisAssembler : private core::Noncopyable<> {
 public:
  DisAssembler(Context* ctx) { }

  void DisAssemble(const Code& code) {
    {
      // code description
      std::ostringstream ss;
      ss << "[code] stack: " << code.stack_depth() << " locals: " << code.locals().size();
      OutputLine(ss.str());
    }
    const Code::Codes& codes = code.codes();
    std::vector<char> line;
    int index = 0;
    std::array<char, 30> buf;
    for (const Instruction* it = code.begin(),
         *last = code.end(); it != last;) {
      const uint32_t opcode = it->GetOP();
      const uint32_t length = kOPLength[opcode];
      const int len = snprintf(buf.data(), buf.size(), "%05d: ", index);
      line.insert(line.end(), buf.data(), buf.data() + len);
      const core::StringPiece piece(OP::String(opcode));
      line.insert(line.end(), piece.begin(), piece.end());
      for (uint32_t first = 1; first < length; ++first) {
        line.push_back(' ');
        std::string val = core::DoubleToStringWithRadix(it[first].value, 10);
        line.insert(line.end(), val.begin(), val.end());
      }
      OutputLine(core::StringPiece(line.data(), line.size()));
      line.clear();
      std::advance(it, length);
      index += length;
    }
    for (Code::Codes::const_iterator it = codes.begin(),
         last = codes.end(); it != last; ++it) {
      DisAssemble(**it);
    }
  }

 private:
  void OutputLine(const core::StringPiece& str) {
    static_cast<Derived*>(this)->OutputLine(str);
  }
};

class OutputDisAssembler : public DisAssembler<OutputDisAssembler> {
 public:
  OutputDisAssembler(Context* ctx, FILE* file)
    : DisAssembler<OutputDisAssembler>(ctx), file_(file) { }

  void OutputLine(const core::StringPiece& str) {
    const std::size_t rv = std::fwrite(str.data(), 1, str.size(), file_);
    if (rv == str.size()) {
      std::fputc('\n', file_);
    }
  }

 private:
  FILE* file_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_DISASSEMBLER_H_
