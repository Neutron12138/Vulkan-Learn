#ifndef __VL_DEBUGUTILS_CPP__
#define __VL_DEBUGUTILS_CPP__

#include "DebugUtils.hpp"
#include "FormatUtils.hpp"

namespace vl
{
    VkBool32
    DebugUtils::debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData)
    {
        ntl::log.logu(
            L"",
            format_debug_message(
                messageSeverity,
                messageTypes,
                pCallbackData));
        return VK_FALSE;
    }

    vk::ResultValue<vk::DebugUtilsMessengerEXT>
    DebugUtils::register_debug_callback(
        DebugUtils *application,
        const vk::Instance &instance,
        vk::DebugUtilsMessageSeverityFlagsEXT severity_flags)
    {
        vk::DebugUtilsMessengerCreateInfoEXT create_info;
        create_info.setMessageSeverity(severity_flags);
        create_info.setMessageType(
            vk::DebugUtilsMessageTypeFlagsEXT(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance));
        create_info.setPfnUserCallback(default_debug_callback);
        create_info.setPUserData(application);

        VkDebugUtilsMessengerEXT messenger;
        vk::Result result;

        auto CreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (CreateDebugUtilsMessengerEXT != nullptr)
        {
            result = static_cast<vk::Result>(
                CreateDebugUtilsMessengerEXT(
                    VkInstance(instance),
                    reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(&create_info),
                    nullptr,
                    &messenger));
        }
        else
        {
            ntl::log.loge(
                L"DebugUtils::register_debug_callback",
                L"Unable to get function \"vkCreateDebugUtilsMessengerEXT\"");
            result = vk::Result::eErrorExtensionNotPresent;
        }

        return vk::ResultValue<vk::DebugUtilsMessengerEXT>(
            result,
            vk::DebugUtilsMessengerEXT(messenger));
    }

    VkBool32 DebugUtils::default_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {
        DebugUtils *application = static_cast<DebugUtils *>(pUserData);
        if (application != nullptr)
            return application->debug_callback(messageSeverity, messageTypes, pCallbackData);
        else
            return VK_FALSE;
    }

    bool
    DebugUtils::destroy_debug_callback(
        vk::DebugUtilsMessengerEXT &messenger,
        const vk::Instance &instance)
    {
        auto func = vkGetInstanceProcAddr(
            instance,
            "vkDestroyDebugUtilsMessengerEXT");

        if (func == nullptr)
        {
            ntl::log.loge(
                L"DebugUtils::destroy_debug_callback",
                L"Unable to get function \"vkDestroyDebugUtilsMessengerEXT\"");
            return false;
        }

        auto DestroyDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(func);

        DestroyDebugUtilsMessengerEXT(
            instance,
            VkDebugUtilsMessengerEXT(messenger),
            nullptr);

        return true;
    }

    ntl::String
    DebugUtils::format_debug_message(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData)
    {
        ntl::StringStream sstr;
        sstr << std::endl
             << "severity:\t" << FormatUtils::format_debug_utils_severity(messageSeverity) << std::endl
             << "type:\t" << FormatUtils::format_debug_utils_type(messageTypes) << std::endl
             << "data:\t" << FormatUtils::format_debug_utils_data(pCallbackData) << std::endl
             << std::endl;
        return sstr.str();
    }

} // namespace vl

#endif