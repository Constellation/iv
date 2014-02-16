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
	GOOGLE_GLOG_INCLUDE_DIRS
	NAMES
	glog/logging.h
	HINTS
	/Library/Frameworks
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	/opt/local/include
	$ENV{SystemDrive}/google-glog/include
	$ENV{ProgramFiles}/google-glog/include
)

set(CMAKE_FIND_FRAMEWORK LAST)

FIND_LIBRARY(
	GOOGLE_GLOG_LIBRARY_DEBUG
	NAMES
	google-glogD libgoogle-glogD
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
	$ENV{SystemDrive}/google-glog/lib
	$ENV{ProgramFiles}/google-glog/lib
)

FIND_LIBRARY(
	GOOGLE_GLOG_LIBRARY_RELEASE
	NAMES
	google-glog libgoogle-glog
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
	$ENV{SystemDrive}/google-glog/lib
	$ENV{ProgramFiles}/google-glog/lib
)

IF (GOOGLE_GLOG_LIBRARY_DEBUG AND NOT GOOGLE_GLOG_LIBRARY_RELEASE)
	SET(GOOGLE_GLOG_LIBRARIES ${GOOGLE_GLOG_LIBRARY_DEBUG})
ENDIF ()

IF (GOOGLE_GLOG_LIBRARY_RELEASE AND NOT GOOGLE_GLOG_LIBRARY_DEBUG)
	SET(GOOGLE_GLOG_LIBRARIES ${GOOGLE_GLOG_LIBRARY_RELEASE})
ENDIF ()

IF (GOOGLE_GLOG_LIBRARY_DEBUG AND GOOGLE_GLOG_LIBRARY_RELEASE)
	SET(GOOGLE_GLOG_LIBRARIES debug ${GOOGLE_GLOG_LIBRARY_DEBUG} optimized ${GOOGLE_GLOG_LIBRARY_RELEASE})
ENDIF ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	google-glog
	DEFAULT_MSG
	GOOGLE_GLOG_INCLUDE_DIRS
	GOOGLE_GLOG_LIBRARIES
)

MARK_AS_ADVANCED(
	GOOGLE_GLOG_INCLUDE_DIRS
	GOOGLE_GLOG_LIBRARIES
	GOOGLE_GLOG_LIBRARY_DEBUG
	GOOGLE_GLOG_LIBRARY_RELEASE
) 
