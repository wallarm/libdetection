include(FindBISON)

FIND_PROGRAM(RE2C_EXECUTABLE NAMES re2c)

if(RE2C_EXECUTABLE)
  macro(RE2C_FILE FILENAME CFILE HFILE)
    set (OUT_CFILE "${CMAKE_CURRENT_BINARY_DIR}/${CFILE}")
    set (OUT_HFILE "${CMAKE_CURRENT_BINARY_DIR}/${HFILE}")
    add_custom_command(
        OUTPUT "${OUT_CFILE}" "${OUT_FHILE}"
        COMMAND "${RE2C_EXECUTABLE}"
        ARGS "-c" "-f"
        "-o" "${OUT_CFILE}"
        "-t" "${OUT_HFILE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
      SET_SOURCE_FILES_PROPERTIES("${OUT_CFILE}" "${OUT_HFILE}" PROPERTIES GENERATED TRUE)
      SET_SOURCE_FILES_PROPERTIES("${OUT_HFILE}" PROPERTIES HEADER_FILE_ONLY TRUE)
  endmacro()
endif()

bison_target(sqli sqli_parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/sqli_parser.c
#  VERBOSE ${CMAKE_CURRENT_BINARY_DIR}/sqli_parser
)

re2c_file(sqli_lexer.re2c sqli_lexer.c sqli_lexer.h)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

set (SOURCES
  sqli.c
  ${BISON_sqli_OUTPUT_SOURCE}
  ${CMAKE_CURRENT_BINARY_DIR}/sqli_lexer.c
)

add_library(${PARSER_TARGET} STATIC ${SOURCES})
set_target_properties(${PARSER_TARGET} PROPERTIES
  POSITION_INDEPENDENT_CODE TRUE)