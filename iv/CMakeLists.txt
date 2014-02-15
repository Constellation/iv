# iv and test

set(LIB_CXX "")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    find_library(LIB_CXX c++)
endif()

set(LIB_M "")
set(LIB_PTHREAD "")
if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    find_library(LIB_M m)
    find_library(LIB_PTHREAD pthread)
endif()

set(IV_LIBRARIES
    ${LIB_CXX}
    ${LIB_M}
    ${LIB_PTHREAD}
    )

include_directories(
    ${PROJECT_SOURCE_DIR}
    third_party
  )

# include test
add_subdirectory(test)

# include subdirectory
add_subdirectory(lv5)
