/*
 * ngfx
 * A 2d rendering library using vulkan with a heavy emphasis on multi viewpoint
 * and headless rendering. It is intended for use with nenbody project
 *
 * The code here is heavily borrowed from these fantastic sources:
 * https://github.com/KhronosGroup/Vulkan-Hpp/tree/master/samples
 * https://github.com/SaschaWillems/Vulkan
 * https://vulkan-tutorial.com/en/
 */

#include "ngfx.hpp"
#include "test_renderer.hpp"

int main() {
  ngfx::TestRenderer app;

  try {
    app.init();
    app.renderTest();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
