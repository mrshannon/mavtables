set (SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/util.cpp"
)
set (HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/util.hpp"
)
target_sources (mavtables 
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/mavtables.cpp"
        ${SOURCES}
        ${HEADERS}
)
target_sources (unit_tests
    PRIVATE
        ${SOURCES}
        ${HEADERS}
)
target_include_directories (mavtables 
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
        "${PROJECT_BINARY_DIR}"
)
target_include_directories (unit_tests
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
        "${PROJECT_BINARY_DIR}"
)
configure_file (
    "${PROJECT_SOURCE_DIR}/src/config.hpp.in"
    "${PROJECT_BINARY_DIR}/config.hpp"
)