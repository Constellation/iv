# Add gtest
ExternalProject_Add(
    download-sunspider
    SVN_REPOSITORY http://svn.webkit.org/repository/webkit/trunk/PerformanceTests/SunSpider
    SVN_REVISION -r 164176
    TIMEOUT 10
    # Disable install step
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    # Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD OFF
    LOG_CONFIGURE OFF
    LOG_BUILD OFF)

add_custom_target(sunspider
    perl
    sunspider
    --shell=${CMAKE_BINARY_DIR}/iv/lv5/lv5
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tools/download-sunspider-prefix/src/download-sunspider/
    )
add_dependencies(sunspider download-sunspider lv5)
