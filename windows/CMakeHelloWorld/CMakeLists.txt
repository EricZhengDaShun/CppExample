# Specify the minimum required version of CMake for this project
cmake_minimum_required(VERSION 3.25)

# Set the C++ standard to C++20 for all targets
# and enforce its usage
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the project name as CMakeHelloWorld
# and specify that we are using C++ language
project(CMakeHelloWorld LANGUAGES CXX)

# Create an executable target named after the project
add_executable(${PROJECT_NAME} )

# Add source files to the executable target
# PRIVATE means these sources are only used by this target
target_sources(
    ${PROJECT_NAME}
    PRIVATE
        main.cpp    # Add the main.cpp source file
)
