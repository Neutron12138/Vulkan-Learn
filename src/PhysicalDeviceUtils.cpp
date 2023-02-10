#ifndef __VL_PHYSICALPhysicalDeviceUtils_CPP__
#define __VL_PHYSICALPhysicalDeviceUtils_CPP__

#include <vector>
#include "PhysicalDeviceUtils.hpp"

namespace vl
{
    template <typename QueueFamilyIndicesType>
    bool
    PhysicalDeviceUtils::is_physical_device_suitable(
        const std::vector<VkQueueFamilyProperties> &families,
        QueueFamilyIndicesType &result)
    {
        return result.find(families);
    }

    template <typename QueueFamilyIndicesType, typename CheckFunc>
    vk::PhysicalDevice
    PhysicalDeviceUtils::pick_suitable_physical_device(
        const vk::Instance &instance,
        QueueFamilyIndicesType &result,
        CheckFunc check_func)
    {
        std::vector<VkPhysicalDevice> devices = get_physical_devices(instance);
        if (devices.size() == 0)
            return vk::PhysicalDevice(nullptr);

        for (auto iter = devices.cbegin(); iter != devices.cend(); iter++)
        {
            vk::PhysicalDevice device = vk::PhysicalDevice(*iter);
            std::vector<VkQueueFamilyProperties> families = get_physical_device_queue_family_properties(device);
            if (check_func(families, result))
                return device;
        }
        return vk::PhysicalDevice(nullptr);
    }

    std::vector<VkPhysicalDevice>
    PhysicalDeviceUtils::get_physical_devices(
        const vk::Instance &instance)
    {
        unsigned int count = 0;
        vkEnumeratePhysicalDevices(
            static_cast<VkInstance>(instance),
            &count,
            nullptr);
        if (count == 0)
        {
            ntl::log.loge(
                NTL_STRING("PhysicalDeviceUtils::get_physical_devices"),
                NTL_STRING("Unable to get any physical devices"));
            return std::vector<VkPhysicalDevice>();
        }

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(
            static_cast<VkInstance>(instance),
            &count,
            devices.data());
        return devices;
    }

    std::vector<VkQueueFamilyProperties>
    PhysicalDeviceUtils::get_physical_device_queue_family_properties(
        const vk::PhysicalDevice &device)
    {
        unsigned int count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            static_cast<VkPhysicalDevice>(device),
            &count,
            nullptr);
        if (count == 0)
        {
            ntl::log.loge(
                NTL_STRING("PhysicalDeviceUtils::get_physical_device_queue_family_properties"),
                NTL_STRING("Unable to get any queue family properties"));
            return std::vector<VkQueueFamilyProperties>();
        }

        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(
            static_cast<VkPhysicalDevice>(device),
            &count,
            families.data());
        return families;
    }

} // namespace vl

#endif