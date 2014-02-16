FILE(
	GLOB
	LIBXML2_INCLUDE_PATHS
	$ENV{HOME}/include/libxml2
	/usr/local/include/libxml2
	/usr/include/libxml2
	$ENV{ProgramW6432}/libxml2*/include
	$ENV{ProgramFiles}/libxml2*/include
)

FILE(
	GLOB
	LIBXML2_LIBRARY_PATHS
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{ProgramW6432}/libxml2*/lib
	$ENV{ProgramFiles}/libxml2*/lib
)

FIND_PATH(
	LIBXML2_INCLUDE_DIRS
	NAMES
	libxml/parser.h
	HINTS
	${LIBXML2_INCLUDE_PATHS}
)

FIND_LIBRARY(
	LIBXML2_LIBRARIES
	NAMES
	libxml2 xml2
	HINTS
	${LIBXML2_LIBRARY_PATHS}
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	LibXml2
	DEFAULT_MSG
	LIBXML2_INCLUDE_DIRS
	LIBXML2_LIBRARIES
)

MARK_AS_ADVANCED(
	LIBXML2_INCLUDE_DIRS
	LIBXML2_LIBRARIES
) 
