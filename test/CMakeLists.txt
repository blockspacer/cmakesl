set(CMAKESL_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKESL_FACADE_DIR})

add_subdirectory(ast)
add_subdirectory(decl_ast)
add_subdirectory(common)
add_subdirectory(errors_observer)
add_subdirectory(errors_observer_mock)
add_subdirectory(exec)
add_subdirectory(lexer)
add_subdirectory(lexer_error)
add_subdirectory(sema)
add_subdirectory(source_location_manipulator)

if(CMAKESL_WITH_TOOLS)
    add_subdirectory(tools)
endif()

if(NOT WIN32)
    add_custom_target(RUN_ALL_TEST COMMAND ${CMAKE_CTEST_COMMAND})
endif()
