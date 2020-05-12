#ifndef CONFIG_H
#define CONFIG_H

#include "ngfx.hpp"

namespace ngfx
{
#if !defined(NDEBUG)
  static bool kDebug = true;
#else
  static bool kDebug = false;
#endif

  static const uint32_t kMaxFramesInFlight = 2;
  static const char * const kValLayers[] = {
    "VK_LAYER_KHRONOS_validation",
  };
  static const uint kValLayerCount = 1;

  static const char * const kDeviceExtensions[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };
  static const uint kDeviceExtensionCount = 1;
}

#endif // CONFIG_H
