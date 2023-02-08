#ifndef __VL_VULKANUTILS_HPP__
#define __VL_VULKANUTILS_HPP__

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include "InstanceUtils.hpp"
#include "DebugUtils.hpp"
#include "PhysicalDeviceUtils.hpp"

namespace vl
{
    /// @brief Vulkan工具
    class VulkanUtils
        : virtual public InstanceUtils,
          virtual public DebugUtils,
          virtual public PhysicalDeviceUtils
    {
    public:
        using SelfType = VulkanUtils;
        using ParentType = ntl::Object;

    public:
        VulkanUtils() noexcept = default;
        explicit VulkanUtils(const SelfType &from) noexcept = default;
        ~VulkanUtils() override = default;

    public:
        SelfType &operator=(const SelfType &from) noexcept = default;
    };
} // namespace vl

#endif