#ifndef __VL_FORMATUTILS_HPP__
#define __VL_FORMATUTILS_HPP__

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <ntl/NTL.hpp>

namespace vl
{
    /// @brief 格式化工具
    class FormatUtils : public ntl::Object
    {
    public:
        using SelfType = FormatUtils;
        using ParentType = ntl::Object;

    public:
        constexpr FormatUtils() noexcept = default;
        constexpr explicit FormatUtils(const SelfType &from) noexcept = default;
        ~FormatUtils() override = default;

    public:
        constexpr SelfType &operator=(const SelfType &from) noexcept = default;

    public:
        /// @brief 格式化消息严重性
        /// @param messageSeverity 消息严重性
        /// @return 格式化后的结果
        static ntl::String format_debug_utils_severity(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity);

        /// @brief 格式化消息类型
        /// @param messageTypes 消息类型
        /// @return 格式化后的结果
        static ntl::String format_debug_utils_type(VkDebugUtilsMessageTypeFlagsEXT messageTypes);

        /// @brief 格式化回调数据
        /// @param pCallbackData 回调数据
        /// @return 格式化后的结果
        static ntl::String format_debug_utils_data(const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData);
    };

} // namespace vl

#endif