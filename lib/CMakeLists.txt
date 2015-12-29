
set (TARGET "detection")
set (TARGET_VERSION_MAJOR 0)
set (TARGET_VERSION_MINOR 0)
set (TARGET_VERSION_PATCH 0)
set (TARGET_VERSION_FULL
  ${TARGET_VERSION_MAJOR}.${TARGET_VERSION_MINOR}.${TARGET_VERSION_PATCH})
set (TARGET_VERSION ${TARGET_VERSION_MAJOR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

set (SOURCES
  detect.c
  detect_util.c
  detect_parser.c
)

set (PARSERS
  sqli
)

set (PARSERS_TARGETS)

foreach (PARSER ${PARSERS})
  set (PARSER_TARGET "${TARGET}_${PARSER}")
  set (PARSERS_TARGETS ${PARSERS_TARGETS} ${PARSER_TARGET})
  add_subdirectory(${PARSER})
endforeach()

add_library(${TARGET} ${SOURCES})
target_link_libraries(${TARGET} avl ${PARSERS_TARGETS})
set_target_properties(${TARGET} PROPERTIES
  VERSION ${TARGET_VERSION_FULL}
  SOVERSION ${TARGET_VERSION}
)
install(TARGETS ${TARGET} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

add_subdirectory(test)
