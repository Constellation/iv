link_directories("${PROJECT_SOURCE_DIR}/third_party/google-benchmark")
include_directories("${PROJECT_SOURCE_DIR}/third_party/google-benchmark/include")

add_executable(aero_benchmarks
  benchmark_aero.cc
  )
target_link_libraries(aero_benchmarks google-benchmark ${IV_LIBRARIES})
