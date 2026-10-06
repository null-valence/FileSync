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
    src/state.cpp
)

target_link_libraries(filesync PRIVATE OpenSSL::Crypto)


include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    )
    
    FetchContent_MakeAvailable(googletest)
    
    enable_testing()
    
    add_executable(filesync_tests
    test/sync_test.cpp
    src/sync.cpp
    src/hasher.cpp
    src/state.cpp
    src/scanner.cpp
)

target_include_directories(filesync_tests PRIVATE src)

target_link_libraries(filesync_tests
    PRIVATE
    GTest::gtest_main
    OpenSSL::Crypto
)

include(GoogleTest)

gtest_discover_tests(filesync_tests)