cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(ngfx VERSION 0.1 LANGUAGES CXX)

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Force c++17 or higher
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.jsom file
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

# Convert relative paths
cmake_policy(SET CMP0076 NEW)

# Make sure freetype is pulled in
find_package(Freetype REQUIRED)

# Make sure -lpthread is pulled in
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# disable lib tests
set(BUILD_TESTS OFF CACHE BOOL "disable tests" FORCE)
set(BUILD_DEMOS OFF CACHE BOOL "disable tests" FORCE)

# find vulkan
find_package(Vulkan REQUIRED)

# Set surface type
if (APPLE)
    add_definitions( -DVK_USE_PLATFORM_MACOS_MVK=1)
    add_definitions( -DVK_USE_PLATFORM_METAL_EXT=1)
    message(STATUS "selected macos as vk platform")
elseif(UNIX)
    add_definitions( -DVK_USE_PLATFORM_XCB_KHR=1)
    message(STATUS "selected xcb as vk platform")
elseif(WIN32)
    message(FATAL_ERROR "win32 not supported")
endif()

# Add subdirs
add_subdirectory("ngfx")
add_subdirectory("lib/glfw")
add_subdirectory("lib/glm")
add_subdirectory("lib/arbor")
