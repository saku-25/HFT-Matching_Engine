cmake_minimum_required(VERSION 3.10)

# Name of the project
project(MatchingEngine)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Tell CMake where to find headers
include_directories(include)

# Create the executable from main, OrderBook, and the SQLite engine
add_executable(matching_engine src/main.cpp src/OrderBook.cpp src/sqlite3.c)