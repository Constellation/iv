#ifndef IV_LV5_BREKAER_COMMAND_H_
#define IV_LV5_BREKAER_COMMAND_H_
#include <iv/file_source.h>
#include <iv/utils.h>
#include <iv/date_utils.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/context.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/railgun/command.h>
#include <iv/lv5/breaker/breaker.h>
namespace iv {
namespace lv5 {
namespace breaker {
namespace detail {

template<typename Source>
railgun::Code* Compile(Context* ctx, const Source& src) {
  AstFactory factory(ctx);
  core::Parser<iv::lv5::AstFactory, Source> parser(&factory,
                                                   src, ctx->symbol_table());
  const FunctionLiteral* const global = parser.ParseProgram();
  railgun::JSScript* script = railgun::JSGlobalScript::New(ctx, &src);
  if (!global) {
    std::fprintf(stderr, "%s\n", parser.error().c_str());
    return NULL;
  }
  return railgun::Compile(ctx, *global, script);
}

static void Execute(const core::StringPiece& data,
                    const std::string& filename, Error* e) {
  breaker::Context ctx;
  ctx.DefineFunction<&Print, 1>("print");
  ctx.DefineFunction<&Quit, 1>("quit");

  core::FileSource src(data, filename);
  railgun::Code* code = Compile(&ctx, src);
  if (!code) {
    return;
  }
  breaker::Compile(code);

  breaker::Run(&ctx, code, e);
}

}  // namespace detail

class TickTimer : private core::Noncopyable<TickTimer> {
 public:
  explicit TickTimer()
    : start_(core::date::HighResTime()) { }

  double GetTime() const {
    return core::date::HighResTime() - start_;
  }
 private:
  double start_;
};

inline JSVal Run(const Arguments& args, Error* e) {
  if (!args.empty()) {
    const JSVal val = args[0];
    if (val.IsString()) {
      const JSString* const f = val.string();
      std::vector<char> buffer;
      const std::string filename(f->GetUTF8());
      if (core::ReadFile(filename, &buffer)) {
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

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREKAER_COMMAND_H_
