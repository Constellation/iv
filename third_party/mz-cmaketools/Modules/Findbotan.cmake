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

IF (NOT WIN32)
   # try using pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   # also fills in BOTAN_DEFINITIONS, although that isn't normally useful
   FIND_PACKAGE(PkgConfig)
   PKG_SEARCH_MODULE(PC_BOTAN botan-1.10 botan-1.9 botan-1.8 botan)
   SET(BOTAN_DEFINITIONS ${PC_BOTAN_CFLAGS})
ENDIF (NOT WIN32)

FIND_PATH(
  BOTAN_INCLUDE_DIRS
  NAMES
  botan/botan.h
  HINTS
  "$ENV{LIB_DIR}/include"
  c:/msys/local/include
  /opt/local/include
  ${PC_BOTAN_INCLUDEDIR}
  ${PC_BOTAN_INCLUDE_DIRS}
)

set(CMAKE_FIND_FRAMEWORK LAST)

FIND_LIBRARY(
  BOTAN_LIBRARIES 
  NAMES 
  botan
  ${PC_BOTAN_LIBRARIES}
  HINTS
  "$ENV{LIB_DIR}/lib"
  #mingw
  c:/msys/local/lib
  ${PC_BOTAN_LIBDIR}
  ${PC_BOTAN_LIBRARY_DIRS}
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	botan
	DEFAULT_MSG
	BOTAN_INCLUDE_DIRS
	BOTAN_LIBRARIES
)

MARK_AS_ADVANCED(
	BOTAN_INCLUDE_DIRS
	BOTAN_LIBRARIES
) 
