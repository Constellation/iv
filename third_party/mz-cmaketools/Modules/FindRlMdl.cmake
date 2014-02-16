FIND_PATH(
	RL_MDL_INCLUDE_DIRS
	NAMES
	rl/mdl/Model.h
	HINTS
	$ENV{HOME}/workspace/rl/src
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	$ENV{USERPROFILE}/workspace/rl/src
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/src"
	$ENV{ProgramFiles}/rl/include
)

FIND_LIBRARY(
	RL_MDL_LIBRARY_DEBUG
	NAMES
	rlmdld
	HINTS
	$ENV{HOME}/workspace/rl/Debug/src/rl/mdl
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/mdl/Debug
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/mdl/Debug"
	$ENV{ProgramFiles}/rl/lib
)

FIND_LIBRARY(
	RL_MDL_LIBRARY_RELEASE
	NAMES
	rlmdl
	HINTS
	$ENV{HOME}/workspace/rl/Release/src/rl/mdl
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/mdl/Release
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/mdl/Release"
	$ENV{ProgramFiles}/rl/lib
)

IF (RL_MDL_LIBRARY_DEBUG AND NOT RL_MDL_LIBRARY_RELEASE)
	SET(RL_MDL_LIBRARIES ${RL_MDL_LIBRARY_DEBUG})
ENDIF (RL_MDL_LIBRARY_DEBUG AND NOT RL_MDL_LIBRARY_RELEASE)

IF (RL_MDL_LIBRARY_RELEASE AND NOT RL_MDL_LIBRARY_DEBUG)
	SET(RL_MDL_LIBRARIES ${RL_MDL_LIBRARY_RELEASE})
ENDIF (RL_MDL_LIBRARY_RELEASE AND NOT RL_MDL_LIBRARY_DEBUG)

IF (RL_MDL_LIBRARY_DEBUG AND RL_MDL_LIBRARY_RELEASE)
	SET(RL_MDL_LIBRARIES debug ${RL_MDL_LIBRARY_DEBUG} optimized ${RL_MDL_LIBRARY_RELEASE})
ENDIF (RL_MDL_LIBRARY_DEBUG AND RL_MDL_LIBRARY_RELEASE)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	RlKin
	DEFAULT_MSG
	RL_MDL_INCLUDE_DIRS
	RL_MDL_LIBRARIES
)

MARK_AS_ADVANCED(
	RL_MDL_INCLUDE_DIRS
	RL_MDL_LIBRARIES
	RL_MDL_LIBRARY_DEBUG
	RL_MDL_LIBRARY_RELEASE
) 
