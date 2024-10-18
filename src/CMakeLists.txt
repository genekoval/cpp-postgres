add_subdirectory(codegen)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

add_subdirectory(libpg++)

if(PROJECT_TESTING)
    add_subdirectory(pg++.test)
endif()

if(BUILD_UTILITIES)
    add_subdirectory(util)
endif()

