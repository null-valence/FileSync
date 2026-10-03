cmake_minimum_required(VERSION 3.20)

project(FileSync VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(filesync
    src/main.cpp
    src/scanner.cpp
    src/sync.cpp
)