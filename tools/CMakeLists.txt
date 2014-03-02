# SunSpider
ExternalProject_Add(
    download-sunspider
    SVN_REPOSITORY http://svn.webkit.org/repository/webkit/trunk/PerformanceTests/SunSpider
    SVN_REVISION -r 164943
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

externalproject_get_property(download-sunspider source_dir)
set(SUNSPIDER_SOURCE_DIR ${source_dir})
get_property(LV5_SHELL TARGET lv5 PROPERTY LOCATION)

add_custom_target(sunspider
    perl
    sunspider
    --shell=${LV5_SHELL}
    --runs=30
    WORKING_DIRECTORY ${SUNSPIDER_SOURCE_DIR}
    )
add_dependencies(sunspider download-sunspider lv5)

add_custom_target(v8-suite
    perl
    sunspider
    --shell=${LV5_SHELL}
    --runs=30
    --v8-suite
    WORKING_DIRECTORY ${SUNSPIDER_SOURCE_DIR}
    )
add_dependencies(v8-suite download-sunspider lv5)

add_custom_target(ubench
    perl
    sunspider
    --shell=${LV5_SHELL}
    --runs=30
    --ubench
    WORKING_DIRECTORY ${SUNSPIDER_SOURCE_DIR}
    )
add_dependencies(v8-suite download-sunspider lv5)

ExternalProject_Add(
    download-test262
    GIT_REPOSITORY https://github.com/Constellation/test262.git
    GIT_TAG 44c4453cf310432519bcd9e399c34c163742096a
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

externalproject_get_property(download-test262 source_dir)
set(TEST262_SOURCE_DIR ${source_dir})
