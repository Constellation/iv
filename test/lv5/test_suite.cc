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
  if (global) {
    lv5::railgun::JSScript* script =
        lv5::railgun::JSGlobalScript::New(ctx, &src);
    return lv5::railgun::CompileIndirectEval(ctx, *global, script);
  } else {
    return NULL;
  }
}

static const char* kPassFileNames[] = {
  "test/lv5/suite/pass000.js",
  "test/lv5/suite/pass001.js",
  "test/lv5/suite/pass002.js",
  "test/lv5/suite/pass004.js",
  "test/lv5/suite/pass005.js",
  "test/lv5/suite/pass006.js",
  "test/lv5/suite/pass007.js",
  "test/lv5/suite/pass008.js",
  "test/lv5/suite/pass009.js",
  "test/lv5/suite/pass010.js",
  "test/lv5/suite/date-parse.js",
  "test/lv5/suite/lhs-assignment.js",
  "test/lv5/suite/ex/log10.js",
  "test/lv5/suite/ex/log2.js",
  "test/lv5/suite/ex/log1p.js",
  "test/lv5/suite/ex/expm1.js",
  "test/lv5/suite/ex/cosh.js",
  "test/lv5/suite/ex/sinh.js",
  "test/lv5/suite/ex/tanh.js",
  "test/lv5/suite/ex/acosh.js",
  "test/lv5/suite/ex/asinh.js",
  "test/lv5/suite/ex/atanh.js",
  "test/lv5/suite/ex/hypot.js",
  "test/lv5/suite/ex/trunc.js",
  "test/lv5/suite/ex/sign.js",
  "test/lv5/suite/ex/cbrt.js",
  "test/lv5/suite/ex/isnan.js",
  "test/lv5/suite/ex/isfinite.js",
  "test/lv5/suite/ex/isinteger.js",
  "test/lv5/suite/ex/tointeger.js",
  "test/lv5/suite/ex/repeat.js",
  "test/lv5/suite/ex/startswith.js",
  "test/lv5/suite/ex/endswith.js",
  "test/lv5/suite/ex/contains.js",
  "test/lv5/suite/ex/toarray.js",
  "test/lv5/suite/ex/reverse.js"
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

