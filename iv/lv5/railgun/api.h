#ifndef IV_LV5_RAILGUN_API_H_
#define IV_LV5_RAILGUN_API_H_
#include <iv/lv5/railgun/context.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline Code* CompileInGlobal(Context* ctx,
                             std::shared_ptr<core::FileSource> src,
                             bool use_folded_registers,
                             Error* e) {
  AstFactory factory(ctx);
  core::Parser<
      AstFactory,
      core::FileSource> parser(&factory, *src.get(), ctx->symbol_table());
  const FunctionLiteral* const global = parser.ParseProgram();
  JSScript* script = JSSourceScript<core::FileSource>::New(ctx, src);
  if (!global) {
    e->Report(
        parser.reference_error() ? Error::Reference : Error::Syntax,
        parser.error());
    return nullptr;
  }
  Code* code = CompileGlobal(ctx, *global, script, use_folded_registers);
  if (!code) {
    e->Report(Error::Syntax, "something wrong");
    return nullptr;
  }
  return code;
}

inline void ExecuteInGlobal(Context* ctx,
                            std::shared_ptr<core::FileSource> src, Error* e) {
  Code* code = CompileInGlobal(ctx, src, false, IV_LV5_ERROR_VOID(e));
  ctx->vm()->Run(code, e);
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_API_H_
