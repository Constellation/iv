#ifndef IV_LV5_RAILGUN_COMMAND_H_
#define IV_LV5_RAILGUN_COMMAND_H_
#include <iv/file_source.h>
#include <iv/utils.h>
#include <iv/date_utils.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/error_check.h>
#include <iv/lv5/context.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/railgun/disassembler.h>
namespace iv {
namespace lv5 {
namespace railgun {
namespace detail {

template<typename Source>
Code* Compile(Context* ctx, const Source& src) {
  AstFactory factory(ctx);
  core::Parser<iv::lv5::AstFactory, Source> parser(&factory,
                                                   src, ctx->symbol_table());
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

#if defined(IV_ENABLE_JIT)
  iv::lv5::breaker::Compile(code);
  iv::lv5::breaker::Run(&ctx, code);
#else
  ctx.vm()->Run(code, e);
#endif
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

inline JSVal Dis(const Arguments& args, Error* e) {
  if (!args.empty()) {
    const JSVal val = args[0];
    if (val.IsObject()) {
      JSObject* obj = val.object();
      if (JSFunction* func = obj->AsCallable()) {
        if (!func->IsNativeFunction()) {
          JSVMFunction* vm_func = static_cast<JSVMFunction*>(func);
          OutputDisAssembler dis(static_cast<Context*>(args.ctx()), stdout);
          dis.DisAssemble(*vm_func->code(), false);
          return JSTrue;
        }
      }
    }
  }
  return JSFalse;
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_COMMAND_H_
