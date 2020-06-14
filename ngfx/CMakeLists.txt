# Add sources
set(
  SOURCES
    "src/main.cpp"
    "src/context.cpp"
    "src/util.cpp"
    "src/swap_data.cpp"
    "src/pipeline.cpp"
    "src/scene.cpp"
)

set(
  HEADERS
    "inc/config.hpp"
    "inc/context.hpp"
    "inc/ngfx.hpp"
    "inc/util.hpp"
    "inc/swap_data.hpp"
    "inc/test_renderer.hpp"
    "inc/pipeline.hpp"
    "inc/scene.hpp"
)

add_executable(ngfx ${SOURCES} ${HEADERS})
target_include_directories(ngfx PUBLIC "inc")
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
