#ifndef IV_LV5_BREKAER_COMMAND_H_
#define IV_LV5_BREKAER_COMMAND_H_
#if defined(IV_ENABLE_JIT)
#include <iv/file_source.h>
#include <iv/utils.h>
#include <iv/date_utils.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/context_fwd.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/railgun/command.h>
#include <iv/lv5/breaker/breaker.h>
#include <iv/lv5/breaker/api.h>
namespace iv {
namespace lv5 {
namespace breaker {
namespace detail {

static void Execute(const core::StringPiece& data,
                    const std::string& filename, Error* e) {
  breaker::Context ctx;
  ctx.DefineFunction<&Print, 1>("print");
  ctx.DefineFunction<&Quit, 1>("quit");
  std::shared_ptr<core::FileSource> src(new core::FileSource(data, filename));
  breaker::ExecuteInGlobal(&ctx, src, e);
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
#endif  // IV_ENABLE_JIT
#endif  // IV_LV5_BREKAER_COMMAND_H_
