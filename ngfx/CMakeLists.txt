# Add sources
set(
    SOURCES
        "src/main.cpp"
)
add_executable(ngfx ${SOURCES})
target_include_directories(ngfx PUBLIC "include")
target_include_directories(ngfx PRIVATE Vulkan::Vulkan)
target_link_libraries(ngfx glfw glm Vulkan::Vulkan)


# Add resources
file(COPY "assets" DESTINATION "./")

# Add shaders
file(COPY "shaders" DESTINATION "./")

install(
    TARGETS ngfx
    ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/lib
    LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/lib
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin
)
