# - Try to find libwallarmmisc library
# Once done this will define
#  LIBWALLARMMISC_FOUND - System has libwallarmmisc
#  LIBWALLARMMISC_INCLUDE_DIRS - The libwallarmmisc include directories
#  LIBWALLARMMISC_LIBRARIES - The libraries needed to use libwallarmmisc

find_path(LIBWALLARMMISC_INCLUDE_DIR wallarm/tree.h)
find_path(LIBWALLARMMISC_CONFIG_INCLUDE_DIR wallarm/config.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  LibWallarmMisc
  REQUIRED_VARS LIBWALLARMMISC_INCLUDE_DIR LIBWALLARMMISC_CONFIG_INCLUDE_DIR)
mark_as_advanced(LIBWALLARMMISC_INCLUDE_DIR)
set (LIBWALLARMMISC_LIBRARIES)
set(LIBWALLARMMISC_INCLUDE_DIRS
  "${LIBWALLARMMISC_INCLUDE_DIR}"
  "${LIBWALLARMMISC_CONFIG_INCLUDE_DIR}"
  )
