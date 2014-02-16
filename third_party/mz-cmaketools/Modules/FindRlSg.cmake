FIND_PATH(
	RL_SG_INCLUDE_DIRS
	NAMES
	rl/sg/Scene.h
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
	RL_SG_LIBRARY_DEBUG
	NAMES
	rlsgd
	HINTS
	$ENV{HOME}/workspace/rl/Debug/src/rl/sg
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/sg/Debug
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/sg/Debug"
	$ENV{ProgramFiles}/rl/lib
)

FIND_LIBRARY(
	RL_SG_LIBRARY_RELEASE
	NAMES
	rlsg
	HINTS
	$ENV{HOME}/workspace/rl/Debug/src/rl/sg
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/sg/Release
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/sg/Release"
	$ENV{ProgramFiles}/rl/lib
)

IF (RL_SG_LIBRARY_DEBUG AND NOT RL_SG_LIBRARY_RELEASE)
	SET(RL_SG_LIBRARIES ${RL_SG_LIBRARY_DEBUG})
ENDIF (RL_SG_LIBRARY_DEBUG AND NOT RL_SG_LIBRARY_RELEASE)

IF (RL_SG_LIBRARY_RELEASE AND NOT RL_SG_LIBRARY_DEBUG)
	SET(RL_SG_LIBRARIES ${RL_SG_LIBRARY_RELEASE})
ENDIF (RL_SG_LIBRARY_RELEASE AND NOT RL_SG_LIBRARY_DEBUG)

IF (RL_SG_LIBRARY_DEBUG AND RL_SG_LIBRARY_RELEASE)
	SET(RL_SG_LIBRARIES debug ${RL_SG_LIBRARY_DEBUG} optimized ${RL_SG_LIBRARY_RELEASE})
ENDIF (RL_SG_LIBRARY_DEBUG AND RL_SG_LIBRARY_RELEASE)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	RlSg
	DEFAULT_MSG
	RL_SG_INCLUDE_DIRS
	RL_SG_LIBRARIES
)

MARK_AS_ADVANCED(
	RL_SG_INCLUDE_DIRS
	RL_SG_LIBRARIES
	RL_SG_LIBRARY_DEBUG
	RL_SG_LIBRARY_RELEASE
) 
