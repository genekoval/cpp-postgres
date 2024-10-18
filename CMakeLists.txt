cmake_minimum_required(VERSION 3.27)

option(BUILD_EXAMPLES "Build the example executable" OFF)
option(BUILD_UTILITIES "Build the utilities executable" OFF)
option(ERRCODES "Path to PostreSQL's errcodes.txt file")

include(cmake/modules.cmake)
include(GitVersion)

project(cpp-postgres
    VERSION ${GIT_VERSION_ABBREV}
    DESCRIPTION "An asynchronous PostgreSQL client for C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS NO)

include(ProjectTesting)
include(packages.cmake)

add_executable(codegen)
target_compile_definitions(codegen PRIVATE VERSION=\"${GIT_VERSION_FULL}\")
target_link_libraries(codegen
    PRIVATE
        commline::commline
        ext::ext
        fmt::fmt
)

set(
    errcodes_header
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pg++/except/errcodes.gen.hpp
)

set(
    errcodes_source
    ${CMAKE_CURRENT_BINARY_DIR}/errcodes.gen.cpp
)

if(NOT ERRCODES)
    execute_process(
        COMMAND pg_config --sharedir
        OUTPUT_VARIABLE output
        ERROR_VARIABLE err
        RESULT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )

    if(result EQUAL 0)
        set(ERRCODES ${output}/errcodes.txt)
        message(STATUS "Found PostgreSQL errcodes: ${ERRCODES}")
    elseif(result MATCHES "^[0-9]+$")
        message(FATAL_ERROR
            "'pg_config --sharedir' exited with code (${result}):\n${err}"
        )
    else()
        message(FATAL_ERROR
            "'pg_config --sharedir' failed to run:\n${result}"
        )
    endif()
endif()

add_custom_command(
    OUTPUT ${errcodes_header} ${errcodes_source}
    COMMAND codegen
    ARGS errcodes
        --header ${errcodes_header}
        --source ${errcodes_source}
        ${ERRCODES}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_library(pg++)
add_library(${PROJECT_NAME}::pg++ ALIAS pg++)
target_sources(pg++ PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES ${errcodes_header}
)
target_sources(pg++ PRIVATE ${errcodes_source})
target_link_libraries(pg++
    PUBLIC
        ext::ext
        fmt::fmt
        netcore::netcore
        nlohmann_json::nlohmann_json
        timber::timber
        uuidcpp::uuid++
)

if(BUILD_EXAMPLES)
    add_executable(examples)
    target_compile_definitions(examples PRIVATE VERSION=\"${GIT_VERSION_FULL}\")
    target_link_libraries(examples
        PRIVATE
            commline::commline
            dotenv::dotenv
            ext::ext
            fmt::fmt
            netcore::netcore
            pg++
            timber::timber
    )
endif()

if(PROJECT_TESTING)
    add_executable(pg++.test)
    target_link_libraries(pg++.test
        PRIVATE
            dotenv::dotenv
            GTest::gtest
            pg++
    )
    add_test(
        NAME "Unit Tests"
        COMMAND pg++.test
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif()

if(BUILD_UTILITIES)
    add_executable(util)
    target_compile_definitions(util PRIVATE VERSION=\"${GIT_VERSION_FULL}\")
    target_link_libraries(util
        PRIVATE
            commline::commline
            dotenv::dotenv
            ext::ext
            fmt::fmt
            netcore::netcore
            pg++
            timber::timber
    )
endif()

add_subdirectory(include)
add_subdirectory(src)

include(Sanitizers)
enable_asan()

include(EasyInstall)
easy_install(
    TARGETS
        pg++
    DEPENDENCIES
        ext
        fmt
        netcore
        nlohmann_json
        timber
        uuidcpp
)

if(PROJECT_IS_TOP_LEVEL)
    add_custom_target(docker
        docker build --progress=plain .
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL
    )
endif()
