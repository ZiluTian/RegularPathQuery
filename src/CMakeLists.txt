# Minimum CMake version required
cmake_minimum_required(VERSION 3.30)

# Project name
project(rpqdb)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the source directory for the NFA implementation
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Add the test executable
# add_executable(test_nfa test_nfa.cpp)

# Link the test executable with any required libraries (if applicable)
# For example, if you use a testing framework like Google Test, link it here.
# target_link_libraries(test_nfa gtest gtest_main pthread)

# Enable testing (optional, if you plan to use CTest)
# enable_testing()
# add_test(NAME TestNFA COMMAND test_nfa)