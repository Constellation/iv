cmake_minimum_required(VERSION 2.8)

project(iv C CXX)

# We can specify Release / Debug / None
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# load minimized mz-cmaketools (but partially)
include(third_party/mz-cmaketools/load.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_PATH} ${PROJECT_SOURCE_DIR}/third_party/mz-cmaketools/Modules)
include(ExternalProject)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)  # don't split absolute link paths
    cmake_policy(SET CMP0012 NEW)  # recognize number & boolean literals
    cmake_policy(SET CMP0015 NEW)  # convert relative link paths to absolute
endif(COMMAND cmake_policy)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

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

option(JIT "enable JIT (default on)" ON)
option(LTO "enable clang LTO - Link-Time-Optimization (default on)" OFF)

# In Ubuntu environment, we can specify supc++
if (NOT DEFINED CLANG_CXX_ABI)
  set(CLANG_CXX_ABI "")
endif()
set(CLANG_CXX_ABI "${CLANG_CXX_ABI}" CACHE STRING
    "Specify C++ ABI library to use." FORCE)
set_property(CACHE CLANG_CXX_ABI PROPERTY STRINGS ;${CXXABIS})

# disable jit
if (NOT JIT)
  add_definitions("-DIV_DISABLE_JIT")
endif()

set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELEASE "")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    if (LTO)
      set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O4")
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O4")
    else()
      set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    endif()
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

macro(setup_compiler_definitions)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
      # using Visual Studio C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4250 /wd4355 /wd4800")
    # Suppressed warnings:
    # C4250: 'class1' : inherits 'class2::member' via dominance
    # C4355: 'this' : used in base member initializer list
    # C4800: 'type' : forcing value to bool 'true' or 'false' (performance warning)
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
          "-g3"
          )
      add_definitions(
          "-D__STDC_LIMIT_MACROS"
          "-D__STDC_CONSTANT_MACROS"
          )

      # direct threading
      add_definitions("-DIV_USE_DIRECT_THREADED_CODE")

      if (CMAKE_BUILD_TYPE MATCHES Release)
          add_definitions("-DNDEBUG")
      endif()

      if (CMAKE_BUILD_TYPE MATCHES Debug)
          add_definitions("-Werror")
      endif()

      if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_definitions("-Wno-nested-anon-types")
      endif()
  endif()
endmacro()

# global libraries and compilers

set(LIB_ABI)
set(LIB_CXX)
set(LIB_M)

if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    find_library(LIB_M m)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
    message("clang ${CMAKE_CXX_FLAGS}")
    add_definitions(
        "-ferror-limit=1000"
        "-fcolor-diagnostics"
        )
    if (NOT ("${CLANG_CXX_ABI}" STREQUAL ""))
      set(LIB_ABI "-l${CLANG_CXX_ABI}")
      message("Using special ABI library ${LIB_ABI}")
    endif()
    find_library(LIB_CXX c++)
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

find_package(Threads)

set(IV_LIBRARIES
    ${LIB_M}
    ${LIB_CXX}
    ${LIB_ABI}
    ${CMAKE_THREAD_LIBS_INIT}
    )

# setup google-test target
add_subdirectory(third_party/google-test)
add_subdirectory(third_party/google-benchmark)
add_subdirectory(iv)
add_subdirectory(tools)
