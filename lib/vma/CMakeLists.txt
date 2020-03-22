cmake_minimum_required(VERSION 3.11)
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE VulkanMemoryAllocator-Hpp/)
