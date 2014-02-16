##
# Copyright (c) 2008-2012 Marius Zwicker
# All rights reserved.
# 
# @LICENSE_HEADER_START:Apache@
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# http://www.mlba-team.de
# 
# @LICENSE_HEADER_END:Apache@
##

FIND_PATH(
	OPENCV_CFG_PATH
	NAMES
	OpenCVConfig.cmake
	HINTS
	/usr/local/share/OpenCV
	/usr/share/OpenCV
	$ENV{OpenCV_DIR}
)

include(FindPackageHandleStandardArgs)

if( OPENCV_CFG_PATH )
	## Include the standard CMake script
	include("${OPENCV_CFG_PATH}/OpenCVConfig.cmake")

    ## Search for a specific version
    set(CVLIB_SUFFIX "${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}")

	set( OPENCV_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS} )
	set( OPENCV_LIBRARIES ${OpenCV_LIBRARIES} ${OpenCV_LIBS})
else()
	message("-- Could not find OpenCV, you might have to set an environment")
	message("   variable 'OpenCV_DIR' pointing to the directory of your")
	message("   installation containing the file 'OpenCVConfig.cmake'")
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	OpenCV
	DEFAULT_MSG
	OPENCV_INCLUDE_DIRS
	OPENCV_LIBRARIES
)

MARK_AS_ADVANCED(
	OPENCV_INCLUDE_DIRS
	OPENCV_LIBRARIES
) 

