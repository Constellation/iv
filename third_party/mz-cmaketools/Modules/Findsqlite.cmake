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
  SQLITE_INCLUDE_DIRS
  NAMES
  sqlite3.h
  HINTS
  "$ENV{LIB_DIR}/include"
  "$ENV{LIB_DIR}/include/sqlite"
  c:/msys/local/include
)

set(CMAKE_FIND_FRAMEWORK LAST)

FIND_LIBRARY(
  SQLITE_LIBRARIES
  NAMES 
  sqlite3 sqlite3_i
  HINTS
  "$ENV{LIB_DIR}/lib"
  #mingw
  c:/msys/local/lib
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	sqlite
	DEFAULT_MSG
	SQLITE_INCLUDE_DIRS
	SQLITE_LIBRARIES
)

MARK_AS_ADVANCED(
	SQLITE_INCLUDE_DIRS
	SQLITE_LIBRARIES
) 