#include <gtest/gtest.h>
#include <iv/platform.h>
#include <iv/lv5/lv5.h>
#include <iv/lv5/railgun/command.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/breaker.h>
#include <iv/lv5/breaker/command.h>
using namespace iv;
namespace {

lv5::railgun::Code* Compile(lv5::railgun::Context* ctx,
                            std::shared_ptr<iv::core::FileSource> src) {
  lv5::AstFactory factory(ctx);
  core::Parser<
      lv5::AstFactory,
      iv::core::FileSource> parser(&factory, *src.get(), ctx->symbol_table());
  const lv5::FunctionLiteral* const global = parser.ParseProgram();
  if (global) {
    lv5::railgun::JSScript* script =
        lv5::railgun::JSSourceScript<iv::core::FileSource>::New(ctx, src);
    return lv5::railgun::CompileIndirectEval(ctx, *global, script);
  } else {
    return NULL;
  }
}

static const char* kPassFileNames[] = {
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

static const char* kSpecFileNames[] = {
  "test/lv5/suite/spec/string-object-length.js",
  "test/lv5/suite/spec/error-constructing.js",
  "test/lv5/suite/spec/function-expression-name.js",
  "test/lv5/suite/spec/date-parse.js",
  "test/lv5/suite/spec/int32-min.js",
  "test/lv5/suite/spec/arguments.js",
  "test/lv5/suite/spec/iterator.js",
  "test/lv5/suite/spec/regexp.js",
  "test/lv5/suite/spec/math-trunc.js",
  "test/lv5/suite/spec/lhs-assignment.js",
  "test/lv5/suite/spec/rhs-assignment.js"
};
static const std::size_t kSpecFileNamesSize =
  sizeof(kSpecFileNames) / sizeof(const char*);

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
    std::shared_ptr<core::FileSource> src(
        new core::FileSource(core::StringPiece(res.data(), res.size()), filename));
    lv5::railgun::Code* code = Compile(&ctx, src);
    ASSERT_TRUE(code) << filename;
    lv5::JSVal ret = ctx.vm()->Run(code, &e);
    ASSERT_FALSE(!!e) << filename;
    EXPECT_TRUE(ret.IsBoolean()) << filename;
    EXPECT_TRUE(ret.boolean()) << filename;
  }
}

#if defined(IV_ENABLE_JIT)
// Jasmine Test

static void ExecuteInContext(lv5::breaker::Context* ctx,
                             const std::string& filename,
                             lv5::Error* e) {
  std::vector<char> res;
  ASSERT_TRUE(core::ReadFile(filename, &res));
  std::shared_ptr<core::FileSource> src(
      new core::FileSource(core::StringPiece(res.data(), res.size()), filename));
  lv5::railgun::Code* code = Compile(ctx, src);
  iv::lv5::breaker::Compile(code);
  iv::lv5::breaker::Run(ctx, code, e);
}

TEST(SuiteCase, JasmineTest) {
  lv5::Init();
  lv5::Error e;
  lv5::breaker::Context ctx;
  ctx.DefineFunction<&lv5::Print, 1>("print");
  ctx.DefineFunction<&lv5::Log, 1>("log");
  ctx.DefineFunction<&lv5::Quit, 1>("quit");
  ctx.DefineFunction<&lv5::HiResTime, 0>("HiResTime");
  ctx.DefineFunction<&lv5::breaker::Run, 0>("run");
  ctx.DefineFunction<&lv5::railgun::Dis, 1>("dis");

  ExecuteInContext(&ctx, "test/lv5/suite/resources/jasmine.js", &e);
  ASSERT_FALSE(e);
  ExecuteInContext(&ctx, "test/lv5/suite/resources/ConsoleReporter.js", &e);
  ASSERT_FALSE(e);
  for (std::size_t i = 0; i < kSpecFileNamesSize; ++i) {
    const std::string filename(kSpecFileNames[i]);
    ExecuteInContext(&ctx, filename, &e);
    ASSERT_FALSE(e);
  }
  ExecuteInContext(&ctx, "test/lv5/suite/resources/driver.js", &e);
  EXPECT_FALSE(e);
}
#endif
