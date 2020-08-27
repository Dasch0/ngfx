#ifndef NGFX_H
#define NGFX_H

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

#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#define VULKAN_HPP_TYPESAFE_CONVERSION
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

#endif // NGFX_H
