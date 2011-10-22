#ifndef IV_AERO_DISASSEMBLER_H_
#define IV_AERO_DISASSEMBLER_H_
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include "detail/array.h"
#include "detail/cstdint.h"
#include "noncopyable.h"
#include "stringpiece.h"
#include "aero/op.h"
#include "aero/code.h"
#include "aero/utility.h"
namespace iv {
namespace aero {

template<typename Derived>
class DisAssembler : private core::Noncopyable<> {
 public:
  void DisAssemble(const Code& code) {
    {
      // code description
      std::ostringstream ss;
      ss << "[regexp] captures: ";
      ss << code.captures();
      ss << " counters: ";
      ss << code.counters();
      OutputLine(ss.str());
    }
    std::vector<char> line;
    int index = 0;
    char buf[30];
    for (std::vector<uint8_t>::const_iterator it = code.bytes().begin(),
         last = code.bytes().end(); it != last;) {
      const uint8_t opcode = *it;
      uint32_t length = kOPLength[opcode];
      if (opcode == OP::CHECK_RANGE || opcode == OP::CHECK_RANGE_INVERTED) {
        length += Load4Bytes(it + 1);
      }
      const int len = snprintf(buf, sizeof(buf) - 1, "%05d: ", index);
      assert(len >= 0);  // %05d, so always pass
      line.insert(line.end(), buf, buf + len);
      const core::StringPiece piece(OP::String(opcode));
      line.insert(line.end(), piece.begin(), piece.end());
      for (uint32_t first = 1; first < length; ++first) {
        line.push_back(' ');
        std::string val = core::DoubleToStringWithRadix(*(it + first), 10);
        line.insert(line.end(), val.begin(), val.end());
      }
      OutputLine(core::StringPiece(line.data(), line.size()));
      line.clear();
      std::advance(it, length);
      index += length;
    }
  }

 private:
  void OutputLine(const core::StringPiece& str) {
    static_cast<Derived*>(this)->OutputLine(str);
  }
};

class OutputDisAssembler : public DisAssembler<OutputDisAssembler> {
 public:
  explicit OutputDisAssembler(FILE* file) : file_(file) { }

  void OutputLine(const core::StringPiece& str) {
    const std::size_t rv = std::fwrite(str.data(), 1, str.size(), file_);
    if (rv == str.size()) {
      std::fputc('\n', file_);
    }
  }

 private:
  FILE* file_;
};

} }  // namespace iv::aero
#endif  // IV_AERO_DISASSEMBLER_H_
