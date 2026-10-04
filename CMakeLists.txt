cmake_minimum_required(VERSION 3.20)

project(FileSync VERSION 0.1.0 LANGUAGES CXX)

find_package(OpenSSL REQUIRED)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(filesync
    src/main.cpp
    src/scanner.cpp
    src/sync.cpp
    src/hasher.cpp
)

target_link_libraries(filesync PRIVATE OpenSSL::Crypto)