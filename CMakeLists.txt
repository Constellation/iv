cmake_minimum_required(VERSION 2.8)

project(iv C CXX)

# load minimized mz-cmaketools (but partially)
include(third_party/mz-cmaketools/load.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_PATH} ${PROJECT_SOURCE_DIR}/third_party/mz-cmaketools/Modules)
include(ExternalProject)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)  # don't split absolute link paths
    cmake_policy(SET CMP0012 NEW)  # recognize number & boolean literals
    cmake_policy(SET CMP0015 NEW)  # convert relative link paths to absolute
endif(COMMAND cmake_policy)

# setup google-test target
add_subdirectory(third_party/google-test)

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

# disable jit
# env.Append(CPPDEFINES=['IV_DISABLE_JIT'])

set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELEASE "")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O4")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
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
          # using Clang
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
          add_definitions(
              "-ferror-limit=1000"
              "-fcolor-diagnostics"
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
endmacro()

add_subdirectory(iv)
add_subdirectory(tools)
