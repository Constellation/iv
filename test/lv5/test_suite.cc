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

static const char* kSpecFileNames[] = {
  "test/lv5/suite/spec/string-object-length.js",
  "test/lv5/suite/spec/error-constructing.js",
  "test/lv5/suite/spec/function-expression-name.js",
  "test/lv5/suite/spec/date-parse.js",
  "test/lv5/suite/spec/int32-min.js",
  "test/lv5/suite/spec/arguments.js",
  "test/lv5/suite/spec/iterator.js",
  "test/lv5/suite/spec/regexp.js",
  "test/lv5/suite/spec/math-log10.js",
  "test/lv5/suite/spec/math-log2.js",
  "test/lv5/suite/spec/math-log1p.js",
  "test/lv5/suite/spec/math-expm1.js",
  "test/lv5/suite/spec/math-cosh.js",
  "test/lv5/suite/spec/math-sinh.js",
  "test/lv5/suite/spec/math-tanh.js",
  "test/lv5/suite/spec/math-acosh.js",
  "test/lv5/suite/spec/math-asinh.js",
  "test/lv5/suite/spec/math-atanh.js",
  "test/lv5/suite/spec/math-hypot.js",
  "test/lv5/suite/spec/math-sign.js",
  "test/lv5/suite/spec/math-trunc.js",
  "test/lv5/suite/spec/math-cbrt.js",
  "test/lv5/suite/spec/number-isnan.js",
  "test/lv5/suite/spec/number-isfinite.js",
  "test/lv5/suite/spec/number-isinteger.js",
  "test/lv5/suite/spec/number-toint.js",
  "test/lv5/suite/spec/string-repeat.js",
  "test/lv5/suite/spec/string-startswith.js",
  "test/lv5/suite/spec/string-endswith.js",
  "test/lv5/suite/spec/string-contains.js",
  "test/lv5/suite/spec/string-toarray.js",
  "test/lv5/suite/spec/string-reverse.js",
  "test/lv5/suite/spec/lhs-assignment.js",
  "test/lv5/suite/spec/rhs-assignment.js"
};
static const std::size_t kSpecFileNamesSize =
  sizeof(kSpecFileNames) / sizeof(const char*);

}  // namespace anonymous

#if defined(IV_ENABLE_JIT)
// Jasmine Test

static void ExecuteInBreakerContext(lv5::breaker::Context* ctx,
                                    const std::string& filename,
                                    lv5::Error* e) {
  std::vector<char> res;
  ASSERT_TRUE(core::ReadFile(filename, &res)) << filename;
  std::shared_ptr<core::FileSource> src(
      new core::FileSource(core::StringPiece(res.data(), res.size()), filename));
  lv5::railgun::Code* code = Compile(ctx, src);
  ASSERT_TRUE(code) << filename;
  iv::lv5::breaker::Compile(code);
  iv::lv5::breaker::Run(ctx, code, e);
}

TEST(SuiteCase, BreakerPassTest) {
  lv5::Init();
  lv5::Error e;
  lv5::breaker::Context ctx;
  ctx.DefineFunction<&lv5::Print, 1>("print");
  ctx.DefineFunction<&lv5::Log, 1>("log");
  ctx.DefineFunction<&lv5::Quit, 1>("quit");
  ctx.DefineFunction<&lv5::HiResTime, 0>("HiResTime");
  ctx.DefineFunction<&lv5::breaker::Run, 0>("run");
  ctx.DefineFunction<&lv5::railgun::Dis, 1>("dis");

  ExecuteInBreakerContext(&ctx, "test/lv5/suite/resources/jasmine.js", &e);
  ASSERT_FALSE(e);
  ExecuteInBreakerContext(&ctx, "test/lv5/suite/resources/ConsoleReporter.js", &e);
  ASSERT_FALSE(e);
  for (std::size_t i = 0; i < kSpecFileNamesSize; ++i) {
    const std::string filename(kSpecFileNames[i]);
    ExecuteInBreakerContext(&ctx, filename, &e);
    ASSERT_FALSE(e);
  }
  ExecuteInBreakerContext(&ctx, "test/lv5/suite/resources/driver.js", &e);
  EXPECT_FALSE(e);
}
#endif

static void ExecuteInRailgunContext(lv5::railgun::Context* ctx,
                                    const std::string& filename,
                                    lv5::Error* e) {
  std::vector<char> res;
  ASSERT_TRUE(core::ReadFile(filename, &res)) << filename;
  std::shared_ptr<core::FileSource> src(
      new core::FileSource(core::StringPiece(res.data(), res.size()), filename));
  lv5::railgun::Code* code = Compile(ctx, src);
  ASSERT_TRUE(code) << filename;
  ctx->vm()->Run(code, e);
}

TEST(SuiteCase, RailgunPassTest) {
  lv5::Init();
  lv5::Error e;
  lv5::railgun::Context ctx;
  ctx.DefineFunction<&lv5::Print, 1>("print");
  ctx.DefineFunction<&lv5::Log, 1>("log");
  ctx.DefineFunction<&lv5::Quit, 1>("quit");
  ctx.DefineFunction<&lv5::HiResTime, 0>("HiResTime");
  ctx.DefineFunction<&lv5::railgun::Run, 0>("run");
  ctx.DefineFunction<&lv5::railgun::Dis, 1>("dis");

  ExecuteInRailgunContext(&ctx, "test/lv5/suite/resources/jasmine.js", &e);
  ASSERT_FALSE(e);
  ExecuteInRailgunContext(&ctx, "test/lv5/suite/resources/ConsoleReporter.js", &e);
  ASSERT_FALSE(e);
  for (std::size_t i = 0; i < kSpecFileNamesSize; ++i) {
    const std::string filename(kSpecFileNames[i]);
    ExecuteInRailgunContext(&ctx, filename, &e);
    ASSERT_FALSE(e);
  }
  ExecuteInRailgunContext(&ctx, "test/lv5/suite/resources/driver.js", &e);
  EXPECT_FALSE(e);
}

