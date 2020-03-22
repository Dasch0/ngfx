# Add sources
set(
    SOURCES
        "src/main.cpp"
)
add_executable(ngfx ${SOURCES})
target_include_directories(ngfx PUBLIC "include")
target_link_libraries(ngfx glfw glm vma utils)

# Add resources
file(COPY "assets" DESTINATION "./")
file(COPY "shaders" DESTINATION "./")

install(
    TARGETS ngfx
    ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/lib
    LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/lib
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin
)
