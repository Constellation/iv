FIND_PATH(
	RL_UTIL_INCLUDE_DIRS
	NAMES
	rl/util/Timer.h
	HINTS
	$ENV{HOME}/workspace/rl/src
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	$ENV{USERPROFILE}/workspace/rl/src
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/src"
	$ENV{ProgramFiles}/rl/include
)

IF (WIN32)
	SET(RL_UTIL_DEFINITIONS -D_WIN32_WINNT=0x400)
ENDIF (WIN32)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	RlUtil
	DEFAULT_MSG
	RL_UTIL_INCLUDE_DIRS
)

MARK_AS_ADVANCED(
	RL_UTIL_DEFINITIONS
	RL_UTIL_INCLUDE_DIRS
) 
