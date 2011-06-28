#ifndef _IV_LV5_RAILGUN_COMMAND_H_
#define _IV_LV5_RAILGUN_COMMAND_H_
#include "file_source.h"
#include "lv5/specialized_ast.h"
#include "lv5/date_utils.h"
#include "lv5/context.h"
#include "lv5/railgun.h"
namespace iv {
namespace lv5 {
namespace railgun {
namespace detail {

static bool ReadFile(const std::string& filename, std::vector<char>* out) {
  if (std::FILE* fp = std::fopen(filename.c_str(), "r")) {
    std::array<char, 1024> buf;
    while (const std::size_t len = std::fread(
            buf.data(),
            1,
            buf.size(), fp)) {
      out->insert(out->end(), buf.begin(), buf.begin() + len);
    }
    std::fclose(fp);
    return true;
  } else {
    std::string err("lv5 can't open \"");
    err.append(filename);
    err.append("\"");
    std::perror(err.c_str());
    return false;
  }
}

}  // namespace detail

// some utility function for only railgun VM
inline JSVal StackDepth(const Arguments& args, Error* e) {
  const VM* vm = static_cast<Context*>(args.ctx())->vm();
  return std::distance(vm->stack()->GetBase(), vm->stack()->GetTop());
}

class TickTimer : private core::Noncopyable<TickTimer> {
 public:
  explicit TickTimer()
    : start_(date::HighResTime()) { }

  double GetTime() const {
    return date::HighResTime() - start_;
  }
 private:
  double start_;
};

inline JSVal Run(const Arguments& args, Error* e) {
  TickTimer timer;
  if (args.size() > 0) {
    const JSVal val = args[0];
    if (val.IsString()) {
      const JSString* const f = val.string();
      std::vector<char> buffer;
      std::string filename;
      core::unicode::UTF16ToUTF8(f->begin(),
                                 f->end(), std::back_inserter(filename));
      if (detail::ReadFile(filename, &buffer)) {
        Context* const ctx = static_cast<Context*>(args.ctx());
        VM* const vm = ctx->vm();
        std::shared_ptr<core::FileSource> const src(
            new core::FileSource(
                core::StringPiece(buffer.data(), buffer.size()), filename));
        AstFactory factory(ctx);
        core::Parser<AstFactory, core::FileSource> parser(&factory, *src);
        const FunctionLiteral* const eval = parser.ParseProgram();
        JSScript* script = JSEvalScript<core::FileSource>::New(ctx, src);
        if (!eval) {
          return timer.GetTime();
        }
        Code* code = CompileFunction(ctx, *eval, script);
        vm->RunGlobal(code, IV_LV5_ERROR(e));
      }
    }
  }
  return timer.GetTime();
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_COMMAND_H_
