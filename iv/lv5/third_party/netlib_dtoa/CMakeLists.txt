# Netlib dtoa static library

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
else()
    add_definitions(
        "-Wno-unused-function"
        "-Wno-sign-compare"
        "-Werror=uninitialized"
        )

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_definitions("-Wno-incompatible-pointer-types-discards-qualifiers")
    endif()
endif()

add_library(netlib_dtoa STATIC netlib_dtoa.c)
