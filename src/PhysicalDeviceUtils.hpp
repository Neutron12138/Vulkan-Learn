#ifndef __VL_PHYSICALPhysicalDeviceUtils_HPP__
#define __VL_PHYSICALPhysicalDeviceUtils_HPP__

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <ntl/NTL.hpp>

namespace vl
{
    /// @brief 物理设备工具
    class PhysicalDeviceUtils : public ntl::Object
    {
    public:
        using SelfType = PhysicalDeviceUtils;
        using ParentType = ntl::Object;

    public:
        constexpr PhysicalDeviceUtils() noexcept = default;
        constexpr explicit PhysicalDeviceUtils(const SelfType &from) noexcept = default;
        ~PhysicalDeviceUtils() override = default;

    public:
        constexpr SelfType &operator=(const SelfType &from) noexcept = default;

    public:
        /// @brief 检查物理设备是否合适
        /// @param device 物理设备
        /// @return 物理设备是否合适，永远返回true
        static bool is_physical_device_suitable(const vk::PhysicalDevice &device);

        /// @brief 挑选合适的物理设备
        /// @tparam CheckFunc 检查函数类型
        /// @param check_func 检查函数
        /// @return 合适的物理设备，没有则返回nullptr
        template <typename CheckFunc>
        static vk::PhysicalDevice pick_suitable_physical_device(const vk::Instance &instance, CheckFunc check_func = is_physical_device_suitable);
    };
} // namespace vl

#endif