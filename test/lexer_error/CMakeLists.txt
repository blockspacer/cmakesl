include(${CMAKESL_DIR}/cmake/cmsl_cmake_utils.cmake)

cmsl_add_test(
    NAME
        lexer_error
    SOURCES
        lexer_error_test.cpp
    INCLUDE_DIRS
        ${CMAKESL_SOURCES_DIR}
        ${CMAKESL_TESTS_DIR}
        ${CMAKESL_FACADE_SOURCES_DIR}
    LIBRARIES
        lexer
        errors_observer_mock
)
