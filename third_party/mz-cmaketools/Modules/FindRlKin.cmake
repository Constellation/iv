FIND_PATH(
	RL_KIN_INCLUDE_DIRS
	NAMES
	rl/kin/Kinematics.h
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
	RL_KIN_LIBRARY_DEBUG
	NAMES
	rlkind
	HINTS
	$ENV{HOME}/workspace/rl/Debug/src/rl/kin
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/kin/Debug
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/kin/Debug"
	$ENV{ProgramFiles}/rl/lib
)

FIND_LIBRARY(
	RL_KIN_LIBRARY_RELEASE
	NAMES
	rlkin
	HINTS
	$ENV{HOME}/workspace/rl/Debug/src/rl/kin
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/kin/Release
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/kin/Release"
	$ENV{ProgramFiles}/rl/lib
)

IF (RL_KIN_LIBRARY_DEBUG AND NOT RL_KIN_LIBRARY_RELEASE)
	SET(RL_KIN_LIBRARIES ${RL_KIN_LIBRARY_DEBUG})
ENDIF (RL_KIN_LIBRARY_DEBUG AND NOT RL_KIN_LIBRARY_RELEASE)

IF (RL_KIN_LIBRARY_RELEASE AND NOT RL_KIN_LIBRARY_DEBUG)
	SET(RL_KIN_LIBRARIES ${RL_KIN_LIBRARY_RELEASE})
ENDIF (RL_KIN_LIBRARY_RELEASE AND NOT RL_KIN_LIBRARY_DEBUG)

IF (RL_KIN_LIBRARY_DEBUG AND RL_KIN_LIBRARY_RELEASE)
	SET(RL_KIN_LIBRARIES debug ${RL_KIN_LIBRARY_DEBUG} optimized ${RL_KIN_LIBRARY_RELEASE})
ENDIF (RL_KIN_LIBRARY_DEBUG AND RL_KIN_LIBRARY_RELEASE)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	RlKin
	DEFAULT_MSG
	RL_KIN_INCLUDE_DIRS
	RL_KIN_LIBRARIES
)

MARK_AS_ADVANCED(
	RL_KIN_INCLUDE_DIRS
	RL_KIN_LIBRARIES
	RL_KIN_LIBRARY_DEBUG
	RL_KIN_LIBRARY_RELEASE
) 
