# Minimum CMake version required
cmake_minimum_required(VERSION 3.25)

# Project name
project(rpqdb)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the source directory for the NFA implementation
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

