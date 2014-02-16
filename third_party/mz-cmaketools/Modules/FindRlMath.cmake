FIND_PATH(
	RL_MATH_INCLUDE_DIRS
	NAMES
	rl/math/Real.h
	HINTS
	$ENV{HOME}/workspace/rl/src
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	$ENV{USERPROFILE}/workspace/rl/src
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/src"
	$ENV{ProgramFiles}/rl/include
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	RlMath
	DEFAULT_MSG
	RL_MATH_INCLUDE_DIRS
)

MARK_AS_ADVANCED(
	RL_MATH_INCLUDE_DIRS
) 
