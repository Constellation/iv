# iv and test

set(LIB_CXX "")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    find_library(LIB_CXX c++)
endif()

set(LIB_M "")
if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    find_library(LIB_M m)
endif()

find_package(Threads)

set(IV_LIBRARIES
    ${LIB_CXX}
    ${LIB_M}
    ${CMAKE_THREAD_LIBS_INIT}
    )

include_directories(
    ${PROJECT_SOURCE_DIR}
    third_party
  )

# include test
add_subdirectory(test)

# include subdirectory
add_subdirectory(lv5)
