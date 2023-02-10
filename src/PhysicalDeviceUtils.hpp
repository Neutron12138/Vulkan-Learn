#ifndef __VL_PHYSICALPhysicalDeviceUtils_HPP__
#define __VL_PHYSICALPhysicalDeviceUtils_HPP__

#include "Vulkan.hpp"
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
        /// @return 物理设备是否合适
        template <typename QueueFamilyIndicesType>
        static bool is_physical_device_suitable(const std::vector<VkQueueFamilyProperties> &families, QueueFamilyIndicesType &result);

        /// @brief 挑选合适的物理设备
        /// @tparam CheckFunc 检查函数类型
        /// @param instance 实例
        /// @param check_func 检查函数
        /// @return 合适的物理设备，没有则返回nullptr
        template <typename QueueFamilyIndicesType, typename CheckFunc>
        static vk::PhysicalDevice pick_suitable_physical_device(const vk::Instance &instance, QueueFamilyIndicesType &result, CheckFunc check_func = is_physical_device_suitable<QueueFamilyIndicesType>);

        /// @brief 获取所有物理设备
        /// @param instance 实例
        /// @return 物理设备
        static std::vector<VkPhysicalDevice> get_physical_devices(const vk::Instance &instance);

        /// @brief 获取队列系列属性
        /// @param device 物理设备
        /// @return 队列系列属性
        static std::vector<VkQueueFamilyProperties> get_physical_device_queue_family_properties(const vk::PhysicalDevice &device);
    };
} // namespace vl

#endif