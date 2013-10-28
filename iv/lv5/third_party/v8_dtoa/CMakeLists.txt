# V8 dtoa static library

add_library(v8_dtoa STATIC
    checks.cc
    conversions.cc
    diy-fp.cc
    fast-dtoa.cc
    platform.cc
    utils.cc
    v8-dtoa.cc
    )
