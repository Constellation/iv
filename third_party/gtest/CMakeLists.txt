# google test library

if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    add_definitions("-Wno-unused-private-field")
endif()

add_library(google-test STATIC gtest-all.cc gtest_main.cc)
