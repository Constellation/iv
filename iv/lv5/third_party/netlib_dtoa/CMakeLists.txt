# Netlib dtoa static library

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
else()
    add_definitions(
        "-Wno-unused-function"
        "-Wno-incompatible-pointer-types-discards-qualifiers"
        "-Wno-sign-compare"
        )
endif()

add_library(netlib_dtoa STATIC netlib_dtoa.c)
