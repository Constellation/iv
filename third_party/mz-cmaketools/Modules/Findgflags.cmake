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
	GFLAGS_INCLUDE_DIRS
	NAMES
	gflags/gflags.h
	HINTS
	/Library/Frameworks
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	/opt/local/include
	$ENV{SystemDrive}/gflags/include
	$ENV{ProgramFiles}/gflags/include
)

set(CMAKE_FIND_FRAMEWORK LAST)

FIND_LIBRARY(
	GFLAGS_LIBRARY_DEBUG
	NAMES
	gflagsD libgflagsD
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
	$ENV{SystemDrive}/gflags/lib
	$ENV{ProgramFiles}/gflags/lib
)

FIND_LIBRARY(
	GFLAGS_LIBRARY_RELEASE
	NAMES
	gflags libgflags
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
	$ENV{SystemDrive}/gflags/lib
	$ENV{ProgramFiles}/gflags/lib
)

IF (GFLAGS_LIBRARY_DEBUG AND NOT GFLAGS_LIBRARY_RELEASE)
	SET(GFLAGS_LIBRARIES ${GFLAGS_LIBRARY_DEBUG})
ENDIF ()

IF (GFLAGS_LIBRARY_RELEASE AND NOT GFLAGS_LIBRARY_DEBUG)
	SET(GFLAGS_LIBRARIES ${GFLAGS_LIBRARY_RELEASE})
ENDIF ()

IF (GFLAGS_LIBRARY_DEBUG AND GFLAGS_LIBRARY_RELEASE)
	SET(GFLAGS_LIBRARIES debug ${GFLAGS_LIBRARY_DEBUG} optimized ${GFLAGS_LIBRARY_RELEASE})
ENDIF ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	gflags
	DEFAULT_MSG
	GFLAGS_INCLUDE_DIRS
	GFLAGS_LIBRARIES
)

MARK_AS_ADVANCED(
	GFLAGS_INCLUDE_DIRS
	GFLAGS_LIBRARIES
	GFLAGS_LIBRARY_DEBUG
	GFLAGS_LIBRARY_RELEASE
) 
