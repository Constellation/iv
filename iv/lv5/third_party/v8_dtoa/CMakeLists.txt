# V8 dtoa static library

link_directories(../third_party/netlib_dtoa)

if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.8)
    add_definitions("-Wno-unused-local-typedefs")
endif()

add_library(v8_dtoa STATIC
    checks.cc
    conversions.cc
    diy-fp.cc
    fast-dtoa.cc
    platform.cc
    utils.cc
    v8-dtoa.cc
    )

target_link_libraries(v8_dtoa netlib_dtoa)
