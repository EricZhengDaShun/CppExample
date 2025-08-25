cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CMakeProject LANGUAGES CXX)

add_executable(${PROJECT_NAME} )

target_sources(
    ${PROJECT_NAME}
        PRIVATE
            main.cpp
)