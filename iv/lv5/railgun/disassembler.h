#ifndef IV_LV5_RAILGUN_DISASSEMBLER_H_
#define IV_LV5_RAILGUN_DISASSEMBLER_H_
#include <vector>
#include <sstream>
#include <iv/detail/array.h>
#include <iv/detail/unordered_map.h>
#include <iv/noncopyable.h>
#include <iv/stringpiece.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/op.h>
#include <iv/lv5/railgun/code.h>
#include <iv/lv5/railgun/direct_threading.h>
namespace iv {
namespace lv5 {
namespace railgun {

template<typename Derived>
class DisAssembler : private core::Noncopyable<> {
 public:
  explicit DisAssembler(Context* ctx) { }

  void DisAssembleGlobal(const Code& code) {
    std::ostringstream ss;
    ss << "total code size: " << code.GetData()->size();
    OutputLine(ss.str());
    DisAssemble(code, true);
  }

  void DisAssemble(const Code& code, bool all = true) {
    {
      // code description
      std::ostringstream ss;
      ss << "[code]"
         << " local: " << code.stack_size()
         << " heap: " << code.heap_size()
         << " registers: " << code.registers();
      OutputLine(ss.str());
    }
    const Code::Codes& codes = code.codes();
    std::vector<char> line;
    int index = 0;
    char buf[30];
    for (const Instruction* it = code.begin(),
         *last = code.end(); it != last;) {
      const uint32_t opcode = it->GetOP();
      const uint32_t length = kOPLength[opcode];
      const int len = snprintf(buf, sizeof(buf) - 1, "%06d: ", index);
      assert(len >= 0);  // %05d, so always pass
      line.insert(line.end(), buf, buf + len);
      DisAssemble(code, it, opcode, length, &line);
      OutputLine(core::StringPiece(line.data(), line.size()));
      line.clear();
      std::advance(it, length);
      index += length;
    }
    if (all) {
      for (Code::Codes::const_iterator it = codes.begin(),
           last = codes.end(); it != last; ++it) {
        DisAssemble(**it);
      }
    }
  }

  void DisAssemble(const Code& code,
                   const Instruction* instr,
                   uint32_t opcode,
                   uint32_t length,
                   std::vector<char>* line) {
    const char* op = OP::String(opcode);
    char buf[80];
    int len = 0;
    switch (opcode) {
      case OP::NOP: {
        len = snprintf(buf, sizeof(buf) - 1, "%s", op);
        break;
      }
      case OP::MV: {
        const int r0 = instr[1].i16[0], r1 = instr[1].i16[1];
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d r%d", op, r0, r1);
        break;
      }
      case OP::RETURN: {
        const int r0 = instr[1].i32[0];
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d", op, r0);
        break;
      }
      case OP::BUILD_ENV: {
        const unsigned int size = instr[1].u32[0], mutable_start = instr[1].u32[1];
        len = snprintf(buf, sizeof(buf) - 1, "%s %u %u", op, size, mutable_start);
        break;
      }
      case OP::INSTANTIATE_VARIABLE_BINDING:
      case OP::INSTANTIATE_DECLARATION_BINDING: {
        const unsigned int name = instr[1].u32[0];
        const char* boolean = (instr[1].u32[1]) ? "true" : "false";
        len = snprintf(buf, sizeof(buf) - 1, "%s %u %s", op, name, boolean);
        break;
      }
      case OP::DELETE_NAME:
      case OP::STORE_NAME:
      case OP::LOAD_CONST:
      case OP::INITIALIZE_HEAP_IMMUTABLE: {
        const int r0 = instr[1].ssw.i16[0];
        const unsigned int offset = instr[1].ssw.u32;
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d %u", op, r0, offset);
        break;
      }
      case OP::LOAD_ARGUMENTS: {
        const int r0 = instr[1].i32[0];
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d", op, r0);
        break;
      }
      case OP::DELETE_HEAP:
      case OP::STORE_HEAP:
      case OP::LOAD_HEAP: {
        const int r0 = instr[1].ssw.i16[0];
        const unsigned int name = instr[1].ssw.u32;
        const unsigned int offset = instr[2].u32[0];
        const unsigned int nest = instr[2].u32[1];
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d %u %u %u", op, r0, name, offset, nest);
        break;
      }
      case OP::DELETE_GLOBAL:
      case OP::STORE_GLOBAL:
      case OP::LOAD_GLOBAL: {
        const int r0 = instr[1].ssw.i16[0];
        const unsigned int name = instr[1].ssw.u32;
        const Map* map = instr[2].map;
        const unsigned int offset = instr[3].u32[0];
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d %u %p %u", op, r0, name, map, offset);
        break;
      }
      case OP::DELETE_ELEMENT:
      case OP::STORE_ELEMENT:
      case OP::LOAD_ELEMENT: {
        const int r0 = instr[1].i16[0], r1 = instr[1].i16[1], r2 = instr[1].i16[2];
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d r%d r%d", op, r0, r1, r2);
        break;
      }
      case OP::DELETE_PROP:
      case OP::STORE_PROP_GENERIC:
      case OP::LOAD_PROP_GENERIC:
      case OP::LOAD_PROP: {
        const int r0 = instr[1].ssw.i16[0], r1 = instr[1].ssw.i16[1];
        const unsigned int name = instr[1].ssw.u32;
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d r%d %u", op, r0, r1, name);
        break;
      }
      case OP::STORE_PROP:
      case OP::LOAD_PROP_OWN: {
        const int r0 = instr[1].ssw.i16[0], r1 = instr[1].ssw.i16[1];
        const unsigned int name = instr[1].ssw.u32;
        const Map* map = instr[2].map;
        const unsigned int offset = instr[3].u32[0];
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d r%d %u %p %u", op, r0, r1, name, map, offset);
        break;
      }
      case OP::LOAD_PROP_PROTO: {
        const int r0 = instr[1].ssw.i16[0], r1 = instr[1].ssw.i16[1];
        const unsigned int name = instr[1].ssw.u32;
        const Map* map1 = instr[2].map;
        const Map* map2 = instr[3].map;
        const unsigned int offset = instr[4].u32[0];
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d r%d %u %p %p %u", op, r0, r1, name, map1, map2, offset);
        break;
      }
      case OP::LOAD_PROP_CHAIN: {
        const int r0 = instr[1].ssw.i16[0], r1 = instr[1].ssw.i16[1];
        const unsigned int name = instr[1].ssw.u32;
        const Chain* chain = instr[2].chain;
        const Map* map = instr[3].map;
        const unsigned int offset = instr[4].u32[0];
        len = snprintf(buf, sizeof(buf) - 1, "%s r%d r%d %u %p %p %u", op, r0, r1, name, chain, map, offset);
        break;
      }
      default: {
      }
    }
    line->insert(line->end(), buf, buf + len);
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
    } else {
      std::abort();
    }
    std::fflush(file_);
  }

 private:
  FILE* file_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_DISASSEMBLER_H_
