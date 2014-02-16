FIND_PATH(
	OLA_INCLUDE_DIRS
	NAMES
	ola/OlaClient.h
	HINTS
	$ENV{OLA_DIR}/include
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	$ENV{SystemDrive}/OLA/include
	$ENV{ProgramFiles}/OLA/include
)

FIND_LIBRARY(
	OLA_LIBRARY
	NAMES
	ola
	HINTS
	$ENV{OLA_DIR}/lib
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{SystemDrive}/OLA/lib
	$ENV{ProgramFiles}/OLA/lib
)

FIND_LIBRARY(
        OLACOMMON_LIBRARY
        NAMES
        olacommon
        HINTS
        $ENV{OLA_DIR}/lib
        $ENV{HOME}/lib
        /usr/local/lib
        /usr/lib
        $ENV{SystemDrive}/OLA/lib
        $ENV{ProgramFiles}/OLA/lib
)

FIND_LIBRARY(
	OLASERVER_LIBRARY
	NAMES
	olaserver
	HINTS
	$ENV{OLA_DIR}/lib
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{SystemDrive}/OLA/lib
	$ENV{ProgramFiles}/OLA/lib
)

IF (OLA_LIBRARY AND NOT OLASERVER_LIBRARY)
        SET(OLA_LIBRARIES ${OLA_LIBRARY} ${OLACOMMON_LIBRARY} )
ENDIF ()

IF (OLASERVER_LIBRARY AND NOT OLA_LIBRARY)
	SET(OLA_LIBRARIES ${OLASERVER_LIBRARY})
ENDIF ()

IF (OLA_LIBRARY AND OLASERVER_LIBRARY)
        SET(OLA_LIBRARIES ${OLA_LIBRARY} ${OLACOMMON_LIBRARY} ${OLASERVER_LIBRARY})
ENDIF ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	OLA
	DEFAULT_MSG
	OLA_INCLUDE_DIRS
	OLA_LIBRARIES
)

MARK_AS_ADVANCED(
	OLA_INCLUDE_DIRS
	OLA_LIBRARIES
	OLA_LIBRARY
	OLASERVER_LIBRARY
) 
