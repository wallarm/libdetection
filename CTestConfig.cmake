set(BUILD_SHARED_LIBS ON)
set(CTEST_PROJECT_NAME "PoC-Detect")
set(MEMORYCHECK_COMMAND_OPTIONS  "--tool=memcheck --show-reachable=yes --track-fds=yes --num-callers=32 --memcheck:leak-check=yes --memcheck:leak-resolution=high --error-exitcode=1")
