cmake_minimum_required(VERSION 3.20)
project(LiV VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example programs" OFF)

add_subdirectory(src)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

message(STATUS "BUILD_EXAMPLES: ${BUILD_EXAMPLES}")


if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

install(
        DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h*"  # Matches .h and .hpp files
)