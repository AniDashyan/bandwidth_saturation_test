cmake_minimum_required(VERSION 3.10)
project(bandwidth_test)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the executable
add_executable(bandwidth_test main.cpp)

# Ensure threading support
find_package(Threads REQUIRED)
target_link_libraries(bandwidth_test PRIVATE Threads::Threads)

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC and Clang on Linux
    target_compile_options(bandwidth_test PRIVATE
        -mavx2          # Enable AVX2 instructions
        -O3             # Optimize for performance
        -march=native   # Use native CPU features (optional, for max compatibility)
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # MSVC on Windows
    target_compile_options(bandwidth_test PRIVATE
        /O2             # Optimize (roughly equivalent to -O2)
        /arch:AVX2      # Enable AVX2
    )
endif()

# Ensure Release build uses optimizations
if(CMAKE_BUILD_TYPE MATCHES "Release")
    if(NOT MSVC)
        target_compile_options(bandwidth_test PRIVATE -O3)
    else()
        target_compile_options(bandwidth_test PRIVATE /O2)
    endif()
endif()