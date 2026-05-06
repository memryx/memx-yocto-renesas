cmake_minimum_required(VERSION 3.10)

project(memx_bench VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(memx_bench
    memx_bench.cpp
    dfp.cpp
    mxpack.cpp
)

target_include_directories(memx_bench PRIVATE include)

target_link_libraries(memx_bench PRIVATE pthread memx)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(memx_bench PRIVATE -O3 -DNDEBUG)
else()
    target_compile_options(memx_bench PRIVATE -Wall -Wextra -Wpedantic -g)
endif()

install(TARGETS memx_bench RUNTIME DESTINATION bin)
# cmake_minimum_required(VERSION 3.10)

# project(memx_bench VERSION 1.0)

# set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# include(GNUInstallDirs)

# if(NOT CMAKE_BUILD_TYPE)
#     set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
# endif()

# add_executable(memx_bench
#     memx_bench.cpp
#     dfp.cpp
#     mxpack.cpp
# )

# # Canonical MemryX headers from the recipe
# target_include_directories(memx_bench PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/driver_include
#     ${CMAKE_CURRENT_SOURCE_DIR}/driver_include/common
#     ${CMAKE_CURRENT_SOURCE_DIR}/driver_include/mpu
#     ${CMAKE_CURRENT_SOURCE_DIR}/driver_include/mpuio
#     ${CMAKE_CURRENT_SOURCE_DIR}/driver_include/platform
#     ${CMAKE_CURRENT_SOURCE_DIR}/driver_include/util

#     # Bench-local headers only
#     ${CMAKE_CURRENT_SOURCE_DIR}/include
# )

# if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
#     set(MEMX_SHARED_LIB "${CMAKE_SOURCE_DIR}/../64bit_toolchain/libmemx.so")
# elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
#     set(MEMX_SHARED_LIB "${CMAKE_SOURCE_DIR}/../32bit_toolchain/libmemx.so")
# else()
#     set(MEMX_SHARED_LIB "${CMAKE_SOURCE_DIR}/x86_64_lib/libmemx.so")
# endif()

# if (EXISTS ${MEMX_SHARED_LIB})
#     message(STATUS "Linking with shared library (local): ${MEMX_SHARED_LIB}")
#     target_link_libraries(memx_bench PRIVATE pthread ${MEMX_SHARED_LIB})
# else()
#     message(WARNING "Local libmemx.so not found, falling back to system path")
#     target_link_libraries(memx_bench PRIVATE pthread memx)
# endif()

# if (CMAKE_BUILD_TYPE STREQUAL "Release")
#     target_compile_options(memx_bench PRIVATE -O3 -DNDEBUG)
# else()
#     target_compile_options(memx_bench PRIVATE -Wall -Wextra -Wpedantic -g)
# endif()

# install(TARGETS memx_bench
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )