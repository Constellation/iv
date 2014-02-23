#include "benchmark/benchmark.h"

static void BM_AeroV8Test(benchmark::State& state) {
}
BENCHMARK(BM_AeroV8Test);

int main(int argc, const char** argv) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}
