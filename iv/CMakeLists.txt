# iv and test

include_directories(
    ${PROJECT_SOURCE_DIR}
    third_party
  )

# include test
add_subdirectory(test)

# include subdirectory
add_subdirectory(lv5)
