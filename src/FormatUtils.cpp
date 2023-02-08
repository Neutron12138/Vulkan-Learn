#ifndef __VL_FORMATUTILS_CPP__
#define __VL_FORMATUTILS_CPP__

#include "FormatUtils.hpp"

namespace vl
{
    ntl::String
    FormatUtils::format_debug_utils_severity(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
    {
        switch (messageSeverity)
        {
        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return NTL_STRING("VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT");
            break;

        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return NTL_STRING("VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT");
            break;

        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return NTL_STRING("VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT");
            break;

        case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return NTL_STRING("VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT");
            break;

        default:
            return ntl::StringUtils::to_string(static_cast<long>(messageSeverity));
            break;
        }
    }

    ntl::String
    FormatUtils::format_debug_utils_type(
        VkDebugUtilsMessageTypeFlagsEXT messageTypes)
    {
        switch (messageTypes)
        {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            return NTL_STRING("VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT");
            break;

        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            return NTL_STRING("VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT");
            break;

        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            return NTL_STRING("VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT");
            break;

        default:
            return ntl::StringUtils::to_string(static_cast<long>(messageTypes));
            break;
        }
    }

    ntl::String
    FormatUtils::format_debug_utils_data(
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData)
    {
        ntl::StringStream sstr;
        return sstr.str();
    }
} // namespace vl

#endif