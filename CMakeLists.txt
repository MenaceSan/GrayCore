# cmake --build Debug --target install -j8
cmake_minimum_required(VERSION 3.16)

#############################################
# Project Settings

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(${CMAKE_SOURCE_DIR}/../cmake/SetVersion.cmake) # VERSION

project(graycore VERSION ${VERSION} LANGUAGES CXX)

include(GNUInstallDirs)
include(${CMAKE_SOURCE_DIR}/../cmake/SetDefaultVars.cmake) # PLATFORM, CMAKE_BUILD_TYPE, CMAKE_INSTALL_PREFIX
include(${CMAKE_SOURCE_DIR}/../cmake/SetGlobalFlags.cmake) # Set global flags for all targets below

#############################################
# Threads

set(THREADS_PREFER_PTHREAD_FLAG TRUE)

find_package(Threads REQUIRED)

#############################################
# GrayCore library

# Create Version.h by substitution placeholders in template Version.h.in
# configure_file(include/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h @ONLY)

# Set paths to all the header files that need to be exported
# Collect all   headers
file(GLOB_RECURSE PUBLIC_HDRS
    ${PROJECT_SOURCE_DIR}/include/*.h
)

# Public shared library for GrayCore 
add_library               (graycore SHARED  ${PUBLIC_HDRS})
if(${PLATFORM} STREQUAL "windows")
target_precompile_headers (graycore PRIVATE ${PUBLIC_HDRS})
endif()

set_target_properties     (graycore PROPERTIES PUBLIC_HEADER "${PUBLIC_HDRS}")
target_include_directories(graycore PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

#############################################
# Source directories
add_subdirectory(src)

#############################################
# Installation paths for targets components

install(TARGETS graycore
        EXPORT GrayCoreTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/graycore
)

# Install CMake import files
install(EXPORT GrayCoreTargets DESTINATION cmake
        NAMESPACE Gray:: FILE GrayCore.cmake
)
