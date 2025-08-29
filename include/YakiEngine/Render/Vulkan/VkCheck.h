#pragma once

#include <print>
#include <vulkan/vk_enum_string_helper.h>

#ifndef NDEBUG
#define VK_CHECK(x)                                                     \
do {                                                                \
VkResult err = x;                                               \
if (err) {                                                      \
std::print("Detected Vulkan error: {}", string_VkResult(err)); \
abort();                                                    \
}                                                               \
} while (0)
#else
#   define VK_CHECK(call)  (void)(call)
#endif
