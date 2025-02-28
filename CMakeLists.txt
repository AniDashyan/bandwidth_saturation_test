cmake_minimum_required(VERSION 3.10)
project(BandwidthSaturationTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(bandwidth_test main.cpp)

# Enable AVX2 for GCC/MinGW
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(bandwidth_test PRIVATE -mavx2 -O3)
endif()