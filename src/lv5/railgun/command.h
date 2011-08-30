#ifndef IV_LV5_RAILGUN_COMMAND_H_
#define IV_LV5_RAILGUN_COMMAND_H_
#include "file_source.h"
#include "lv5/specialized_ast.h"
#include "lv5/error_check.h"
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


template<typename Source>
Code* Compile(Context* ctx, const Source& src) {
  AstFactory factory(ctx);
  core::Parser<iv::lv5::AstFactory, Source> parser(&factory, src);
  const FunctionLiteral* const global = parser.ParseProgram();
  JSScript* script = JSGlobalScript::New(ctx, &src);
  if (!global) {
    std::fprintf(stderr, "%s\n", parser.error().c_str());
    return NULL;
  }
  return Compile(ctx, *global, script);
}

static void Execute(const core::StringPiece& data,
                    const std::string& filename, Error* e) {
  Context ctx;
  core::FileSource src(data, filename);
  Code* code = Compile(&ctx, src);
  if (!code) {
    return;
  }
  ctx.DefineFunction<&Print, 1>("print");
  ctx.DefineFunction<&Quit, 1>("quit");
  ctx.vm()->Run(code, e);
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
  if (args.size() > 0) {
    const JSVal val = args[0];
    if (val.IsString()) {
      const JSString* const f = val.string();
      std::vector<char> buffer;
      const std::string filename(f->GetUTF8());
      if (detail::ReadFile(filename, &buffer)) {
        TickTimer timer;
        detail::Execute(
            core::StringPiece(buffer.data(), buffer.size()),
            filename, IV_LV5_ERROR(e));
        return timer.GetTime();
      }
    }
  }
  return JSUndefined;
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_COMMAND_H_
