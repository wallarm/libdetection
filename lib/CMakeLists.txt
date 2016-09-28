
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
  pt
)

set (PARSERS_TARGETS)

foreach (PARSER ${PARSERS})
  set (PARSER_TARGET "${TARGET}_${PARSER}")
  set (PARSERS_TARGETS ${PARSERS_TARGETS} ${PARSER_TARGET})
  add_subdirectory(${PARSER})
endforeach()

if (ENABLE_SHARED)
  set (SHARED_TARGET ${TARGET})
  if (ENABLE_STATIC)
    set (STATIC_TARGET ${TARGET}_static)
  else()
    set (STATIC_TARGET)
  endif()
else()
  set (SHARED_TARGET)
  set (STATIC_TARGET ${TARGET})
endif()

set (LIB_TARGETS ${STATIC_TARGET} ${SHARED_TARGET})

if (ENABLE_SHARED)
  add_library(${SHARED_TARGET} SHARED ${SOURCES})
  target_link_libraries(${SHARED_TARGET} avl ${PARSERS_TARGETS})
  set_target_properties(${SHARED_TARGET} PROPERTIES
    VERSION ${TARGET_VERSION_FULL}
    SOVERSION ${TARGET_VERSION}
  )
endif()
if (ENABLE_STATIC)
  add_library(${STATIC_TARGET} STATIC ${SOURCES})
  target_link_libraries(${STATIC_TARGET} avl ${PARSERS_TARGETS})
  set_target_properties(${STATIC_TARGET} PROPERTIES OUTPUT_NAME ${TARGET})
endif()
install(TARGETS ${LIB_TARGETS}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ../include/detect DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory(test)
