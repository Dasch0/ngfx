# Add sources
set(
    SOURCES
        "src/main.cpp"
        "src/math.cpp"
        "src/utils.cpp"
)
add_executable(ngfx ${SOURCES})
target_include_directories(ngfx PUBLIC "include")
target_link_libraries(ngfx glfw glm Vulkan-Headers Vulkan::Vulkan vma)

install(
    TARGETS ngfx
    ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/lib
    LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/lib
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin
)
