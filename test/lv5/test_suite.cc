#include <gtest/gtest.h>
#include <fstream>
#include <iv/platform.h>
#include <iv/lv5/lv5.h>
#include <iv/lv5/railgun/command.h>
#include <iv/lv5/railgun/railgun.h>
#if defined(IV_ENABLE_JIT)
#include <iv/lv5/breaker/breaker.h>
#include <iv/lv5/breaker/command.h>
#endif  // defined(IV_ENABLE_JIT)
using namespace iv;
namespace {

lv5::railgun::Code* Compile(lv5::railgun::Context* ctx,
                            std::shared_ptr<iv::core::FileSource> src,
                            bool use_folded_registers) {
  lv5::AstFactory factory(ctx);
  core::Parser<
      lv5::AstFactory,
      iv::core::FileSource> parser(&factory, *src.get(), ctx->symbol_table());
  const lv5::FunctionLiteral* const global = parser.ParseProgram();
  if (global) {
    lv5::railgun::JSScript* script =
        lv5::railgun::JSSourceScript<iv::core::FileSource>::New(ctx, src);
    return lv5::railgun::CompileIndirectEval(ctx, *global, script, use_folded_registers);
  } else {
    return nullptr;
  }
}

static std::vector<std::string> GetTests() {
  const std::string prefix("test/lv5/suite/");
  std::vector<std::string> vec;
  std::ifstream stream((prefix + "spec.list").c_str());
  std::string line;
  while (std::getline(stream, line)) {
    vec.push_back(prefix + line);
  }
  return vec;
}

}  // namespace anonymous

#if defined(IV_ENABLE_JIT)
// Jasmine Test

static void ExecuteInBreakerContext(lv5::breaker::Context* ctx,
                                    const std::string& filename,
                                    lv5::Error* e) {
  std::vector<char> res;
  ASSERT_TRUE(core::io::ReadFile(filename, &res)) << filename;
  std::shared_ptr<core::FileSource> src(
      new core::FileSource(core::StringPiece(res.data(), res.size()), filename));
  lv5::railgun::Code* code = Compile(ctx, src, true);
  ASSERT_TRUE(code) << filename;
  iv::lv5::breaker::Compile(ctx, code);
  iv::lv5::breaker::Run(ctx, code, e);
}

TEST(SuiteCase, BreakerPassTest) {
  lv5::Init();
  lv5::Error::Standard e;
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
  const std::vector<std::string> files = GetTests();
  for (std::vector<std::string>::const_iterator it = files.begin(),
       last = files.end(); it != last; ++it) {
    ExecuteInBreakerContext(&ctx, *it, &e);
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
  ASSERT_TRUE(core::io::ReadFile(filename, &res)) << filename;
  std::shared_ptr<core::FileSource> src(
      new core::FileSource(core::StringPiece(res.data(), res.size()), filename));
  lv5::railgun::Code* code = Compile(ctx, src, false);
  ASSERT_TRUE(code) << filename;
  ctx->vm()->Run(code, e);
}

TEST(SuiteCase, RailgunPassTest) {
  lv5::Init();
  lv5::Error::Standard e;
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
  const std::vector<std::string> files = GetTests();
  for (std::vector<std::string>::const_iterator it = files.begin(),
       last = files.end(); it != last; ++it) {
    ExecuteInRailgunContext(&ctx, *it, &e);
    ASSERT_FALSE(e);
  }
  ExecuteInRailgunContext(&ctx, "test/lv5/suite/resources/driver.js", &e);
  EXPECT_FALSE(e);
}

