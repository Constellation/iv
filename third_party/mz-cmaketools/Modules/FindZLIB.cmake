FILE(
	GLOB
	ZLIB_INCLUDE_PATHS
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	$ENV{ProgramW6432}/zlib*/include
	$ENV{ProgramFiles}/zlib*/include
)

FILE(
	GLOB
	ZLIB_LIBRARY_PATHS
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{ProgramW6432}/zlib*/lib
	$ENV{ProgramFiles}/zlib*/lib
)

FIND_PATH(
	ZLIB_INCLUDE_DIRS
	NAMES
	zlib.h
	HINTS
	${ZLIB_INCLUDE_PATHS}
)

FIND_LIBRARY(
	ZLIB_LIBRARIES
	NAMES
	z zlib zlib_a zlibstat zlibwapi zdll
	HINTS
	${ZLIB_LIBRARY_PATHS}
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	ZLIB
	DEFAULT_MSG
	ZLIB_INCLUDE_DIRS
	ZLIB_LIBRARIES
)

MARK_AS_ADVANCED(
	ZLIB_INCLUDE_DIRS
	ZLIB_LIBRARIES
) 
