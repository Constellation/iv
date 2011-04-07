#ifndef _IV_LV5_RAILGUN_DISASSEMBLER_H_
#define _IV_LV5_RAILGUN_DISASSEMBLER_H_
#include <iostream>  // NOLINT
#include <vector>
#include <tr1/array>
#include <tr1/cstdio>
#include "noncopyable.h"
#include "stringpiece.h"
#include "lv5/railgun_fwd.h"
#include "lv5/railgun_op.h"
#include "lv5/railgun_code.h"
namespace iv {
namespace lv5 {
namespace railgun {

template<typename Derived>
class DisAssembler : private core::Noncopyable<DisAssembler<Derived> >::type {
 public:
  DisAssembler() { }

  void DisAssemble(const Code& code) {
    typedef typename Code::Data Data;
    OutputLine("[code]");
    const Data& data = code.Main();
    const Code::Codes& codes = code.codes();
    std::vector<char> line;
    int index = 0;
    std::tr1::array<char, 30> buf;
    for (Data::const_iterator it = data.begin(),
         last = data.end(); it != last; ++it, ++index) {
      const uint8_t opcode = *it;
      const bool has_arg = OP::HasArg(opcode);
      const int len = snprintf(buf.data(), buf.size(), "%05d: ", index);
      uint16_t oparg = 0;
      if (has_arg) {
        oparg = *(++it);
        oparg += ((*(++it)) << 8);
        index += 2;
      }
      line.insert(line.end(), buf.data(), buf.data() + len);
      const core::StringPiece piece(OP::String(opcode));
      line.insert(line.end(), piece.begin(), piece.end());
      line.push_back(' ');
      if (has_arg) {
        std::string val = core::DoubleToStringWithRadix(oparg, 10);
        line.insert(line.end(), val.begin(), val.end());
      }
      OutputLine(core::StringPiece(line.data(), line.size()));
      line.clear();
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

class CoutDisAssembler : public DisAssembler<CoutDisAssembler> {
 public:
  void OutputLine(const core::StringPiece& str) {
    std::cout.write(str.data(), str.size());
    std::cout << std::endl;
  }
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_DISASSEMBLER_H_
