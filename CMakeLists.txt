cmake_minimum_required(VERSION 2.8)

project(iv)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)  # don't split absolute link paths
    cmake_policy(SET CMP0012 NEW)  # recognize number & boolean literals
    cmake_policy(SET CMP0015 NEW)  # convert relative link paths to absolute
endif(COMMAND cmake_policy)

include_directories(
    .
    iv/third_party
    /usr/local/include
    /opt/local/include
    )

macro(include_if_exists directory)
    if(EXISTS "${directory}/")
        include_directories(${directory})
    endif()
endmacro()

macro(link_directories_if_exists directory)
    if(EXISTS "${directory}/")
        link_directories(${directory})
    endif()
endmacro()

# include local directories
include_if_exists(/usr/local/include)
include_if_exists(/opt/local/include)

# disable jit
# env.Append(CPPDEFINES=['IV_DISABLE_JIT'])

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
    include_directories(win/gc)
    link_directories(win/gc)
else()
    add_definitions("-pipe")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-operator-names")
    add_definitions(
        "-Wall"
        "-Wextra"
        "-Wno-unused-parameter"
        "-Wwrite-strings"
        "-Wreturn-type"
        "-Wpointer-arith"
        "-Wno-unused-variable"
        "-Wwrite-strings"
        "-Wno-long-long"
        "-Wno-missing-field-initializers"
        )
    add_definitions(
        "-D__STDC_LIMIT_MACROS"
        "-D__STDC_CONSTANT_MACROS"
        )

    # direct threading
    add_definitions("-DIV_USE_DIRECT_THREADED_CODE")

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror -g3")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -O4")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O4")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -O3")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    endif()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # using Clang
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        add_definitions(
            "-ferror-limit=1000"
            "-stdlib=libc++"
            "-Wno-nested-anon-types"
            )
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # using GCC
        execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                        OUTPUT_VARIABLE GCC_VERSION)
        if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        endif()
    endif()
endif()

add_subdirectory(iv/lv5)
add_subdirectory(test)
