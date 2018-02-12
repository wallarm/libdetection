find_package(LibWallarmMisc REQUIRED)

set (TARGET "libdetection_perf")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${LIBWALLARMMISC_INCLUDE_DIRS}
)

set (SOURCES
  perf.c
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/vg.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vg" @ONLY)

add_executable(${TARGET} ${SOURCES})
target_link_libraries(${TARGET} detection)
install(TARGETS ${TARGET}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
