# SunSpider
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

externalproject_get_property(download-sunspider source_dir)
set(SUNSPIDER_SOURCE_DIR ${source_dir})
get_property(SUNSPIDER_SHELL TARGET lv5 PROPERTY LOCATION)
add_custom_target(sunspider
    perl
    sunspider
    --shell=${SUNSPIDER_SHELL}
    --runs=30
    WORKING_DIRECTORY ${SUNSPIDER_SOURCE_DIR}
    )
add_dependencies(sunspider download-sunspider lv5)
