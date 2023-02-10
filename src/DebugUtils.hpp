#ifndef __VL_DEBUGUTILS_HPP__
#define __VL_DEBUGUTILS_HPP__

#include "Vulkan.hpp"
#include <ntl/NTL.hpp>

namespace vl
{
    /// @brief 调试工具
    class DebugUtils : public ntl::Object
    {
    public:
        using SelfType = DebugUtils;
        using ParentType = ntl::Object;

        /// @brief 调试信息回调函数类型
        using DebugCallbackType = VkBool32 (*)(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

    public:
        constexpr DebugUtils() noexcept = default;
        constexpr explicit DebugUtils(const SelfType &from) noexcept = default;
        ~DebugUtils() override = default;

    public:
        constexpr SelfType &operator=(const SelfType &from) noexcept = default;

    public:
        /// @brief 调试信息回调函数
        /// @param messageSeverity 消息严重性
        /// @param messageTypes 消息类型
        /// @param pCallbackData 回调数据
        /// @return 是否终止验证层
        virtual VkBool32 debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData);

    public:
        /// @brief 注册调试信息回调函数
        /// @param self 自身
        /// @param instance 实例
        /// @param severity_flags 严重性标志
        /// @return 结果
        static vk::ResultValue<vk::DebugUtilsMessengerEXT>
        register_debug_callback(
            DebugUtils *self,
            const vk::Instance &instance,
            vk::DebugUtilsMessageSeverityFlagsEXT severity_flags =
                vk::DebugUtilsMessageSeverityFlagsEXT(
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError));

        /// @brief 默认调试信息回调函数
        /// @param messageSeverity 消息严重性
        /// @param messageTypes 消息类型
        /// @param pCallbackData 回调数据
        /// @param pUserData 用户数据
        /// @return 是否终止验证层
        static VkBool32 default_debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

        /// @brief 销毁调试信息回调
        /// @param messenger 回调
        /// @param instance 实例
        /// @return 是否成功
        static bool destroy_debug_callback(vk::DebugUtilsMessengerEXT &messenger, const vk::Instance &instance);

        /// @brief 格式化调试信息
        /// @param messageSeverity 消息严重性
        /// @param messageTypes 消息类型
        /// @param pCallbackData 回调数据
        /// @return 格式化后的结果
        static ntl::String
        format_debug_message(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData);
    };

} // namespace vl

#endif