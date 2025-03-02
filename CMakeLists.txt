cmake_minimum_required(VERSION 3.10)
project(bandwidth_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bandwidth_test main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(bandwidth_test PRIVATE Threads::Threads)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bandwidth_test PRIVATE -O3)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(bandwidth_test PRIVATE /O2)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
    if(NOT MSVC)
        target_compile_options(bandwidth_test PRIVATE -O3)
    else()
        target_compile_options(bandwidth_test PRIVATE /O2)
    endif()
endif()