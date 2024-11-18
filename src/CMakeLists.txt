cmake_minimum_required(VERSION 3.15)

set(SOURCES
    main.cpp
)

add_executable(kaleidoscope ${SOURCES})
include_directories(${CMAKE_SOURCE_DIR}/src/parser)
target_compile_options(kaleidoscope PRIVATE -Wall -Wextra -Wpedantic)