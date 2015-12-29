set (UNIT_SOURCES
  detect_unit.c
)

set (UNIT_TARGET "unit")

add_executable(${UNIT_TARGET} ${UNIT_SOURCES})
target_link_libraries(${UNIT_TARGET} ${TARGET} cunit)
add_test(NAME ${UNIT_TARGET} COMMAND ${UNIT_TARGET})
