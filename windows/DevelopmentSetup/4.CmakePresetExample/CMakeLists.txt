cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CmakePresetExample LANGUAGES CXX)

find_package(fmt CONFIG REQUIRED)

add_executable(${PROJECT_NAME} )

target_sources(
    ${PROJECT_NAME}
    PRIVATE
        main.cpp
)

target_link_libraries(
    ${PROJECT_NAME} 
        PRIVATE 
            fmt::fmt
)