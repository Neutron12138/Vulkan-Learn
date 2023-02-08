#ifndef __VL_PHYSICALPhysicalDeviceUtils_CPP__
#define __VL_PHYSICALPhysicalDeviceUtils_CPP__

#include <vector>
#include "PhysicalDeviceUtils.hpp"

namespace vl
{
    bool
    PhysicalDeviceUtils::is_physical_device_suitable(
        const vk::PhysicalDevice &device)
    {
        return true;
    }

    template <typename CheckFunc>
    vk::PhysicalDevice
    PhysicalDeviceUtils::pick_suitable_physical_device(
        const vk::Instance &instance,
        CheckFunc check_func)
    {
        unsigned int count = 0;
        vkEnumeratePhysicalDevices(
            static_cast<VkInstance>(instance),
            &count,
            nullptr);
        if (count == 0)
        {
            ntl::log.loge(
                L"PhysicalDeviceUtils::pick_suitable_device",
                L"Unable to find any physical devices");
            return vk::PhysicalDevice(nullptr);
        }

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(
            static_cast<VkInstance>(instance),
            &count,
            devices.data());

        for (auto iter = devices.cbegin(); iter != devices.cend(); iter++)
        {
            vk::PhysicalDevice device = vk::PhysicalDevice(*iter);
            if (check_func(device))
                return device;
        }
        return vk::PhysicalDevice(nullptr);
    }
} // namespace vl

#endif