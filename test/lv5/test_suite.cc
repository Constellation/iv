#include <gtest/gtest.h>
#include <iv/lv5/lv5.h>
#include <iv/lv5/railgun/command.h>
using namespace iv;
namespace {

template<typename Source>
lv5::railgun::Code* Compile(lv5::railgun::Context* ctx, const Source& src) {
  lv5::AstFactory factory(ctx);
  core::Parser<lv5::AstFactory, Source> parser(
      &factory, src, ctx->symbol_table());
  const lv5::FunctionLiteral* const global = parser.ParseProgram();
  lv5::railgun::JSScript* script =
      lv5::railgun::JSGlobalScript::New(ctx, &src);
  return lv5::railgun::CompileIndirectEval(ctx, *global, script);
}

static const char* kPassFileNames[] = {
  "test/lv5/suite/pass000.js",
  "test/lv5/suite/pass001.js",
  "test/lv5/suite/pass002.js",
  "test/lv5/suite/pass004.js"
};
static const std::size_t kPassFileNamesSize =
  sizeof(kPassFileNames) / sizeof(const char*);

}  // namespace anonymous

TEST(SuiteCase, PassTest) {
  lv5::Init();
  for (std::size_t i = 0; i < kPassFileNamesSize; ++i) {
    const std::string filename(kPassFileNames[i]);
    std::vector<char> res;
    ASSERT_TRUE(core::ReadFile(filename, &res));
    lv5::Error e;
    lv5::railgun::Context ctx;
    ctx.DefineFunction<&lv5::Print, 1>("print");
    ctx.DefineFunction<&lv5::Quit, 1>("quit");
    ctx.DefineFunction<&lv5::HiResTime, 0>("HiResTime");
    ctx.DefineFunction<&lv5::railgun::Run, 0>("run");
    ctx.DefineFunction<&lv5::railgun::StackDepth, 0>("StackDepth");
    ctx.DefineFunction<&lv5::railgun::Dis, 1>("dis");
    core::FileSource src(core::StringPiece(res.data(), res.size()), filename);
    lv5::railgun::Code* code = Compile(&ctx, src);
    ASSERT_TRUE(code) << filename;
    lv5::JSVal ret = ctx.vm()->Run(code, &e);
    ASSERT_FALSE(!!e) << filename;
    EXPECT_TRUE(ret.IsBoolean()) << filename;
    EXPECT_TRUE(ret.boolean()) << filename;
  }
}

