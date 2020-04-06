cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(ngfx VERSION 0.1 LANGUAGES CXX)

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Force c++11 or higher
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
# set use local vulkan headers
set (VULKAN_HEADERS_INSTALL_DIR "/lib/Vulkan-Headers" FORCE)

# Set surface type
if (APPLE)
    add_definitions( -DVK_USE_PLATFORM_MACOS_MVK=1)
    add_definitions( -DVK_USE_PLATFORM_METAL_EXT=1)
    add_definitions( -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
    message(STATUS "selected macos as vk platform")
elseif(UNIX)
    add_definitions( -DVK_USE_PLATFORM_XCB_KHR=1)
    add_definitions( -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
    message(STATUS "selected xcb as vk platform")
elseif(WIN32)
    message(FATAL_ERROR "win32 not supported")
endif()

# Add subdirs
add_subdirectory("ngfx")
add_subdirectory("lib/glfw")
add_subdirectory("lib/glm")
add_subdirectory("lib/Vulkan-Headers")

# Add libraries without CMakelists.txt
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE "lib/VulkanMemoryAllocator/src")
