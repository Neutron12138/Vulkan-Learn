#ifndef __VL_VULKANAPPLICATION_HPP__
#define __VL_VULKANAPPLICATION_HPP__

#include <vector>
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <ntl/NTL.hpp>
#include <nwl/NWL.hpp>

namespace vl
{
    /// @brief Vulkan程序
    class VulkanApplication : public ntl::Application
    {
    public:
        using SelfType = VulkanApplication;
        using ParentType = ntl::Application;

        /// @brief 调试信息回调函数类型
        using DebugCallbackType = VkBool32 (*)(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

    protected:
        /// @brief 窗口
        nwl::SFMLWindow m_window;

    public:
        VulkanApplication() = default;
        explicit VulkanApplication(const SelfType &from) = default;
        ~VulkanApplication() override = default;

    public:
        SelfType &operator=(const SelfType &from) = default;

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
        int run() override;
        void onIdle() override;
        void onCreated() override;

    public:
        /// @brief 当绘制时调用
        virtual void onDisplay() = 0;

        /// @brief 创建实例
        /// @return 是否创建成功
        virtual bool CreateInstance() = 0;

        /// @brief 创建调试信息回调
        /// @return 是否创建成功
        virtual bool CreateDebugMessenger()=0;

    public:
        /// @brief 创建一个vk实例
        /// @param name 实例名
        /// @return 结果
        static vk::ResultValue<vk::Instance> create_instance(const std::string &name, const std::vector<std::string> &validation_layers);

        /// @brief 检查层支持
        /// @param validation_layers 验证层
        /// @param layers 所有的层
        /// @return 不支持的曾列表
        static std::vector<std::string> check_layer_support(const std::vector<std::string> &validation_layers, const std::vector<vk::LayerProperties> &layers);

        /// @brief 注册调试信息回调函数
        /// @param debug_callback
        static vk::ResultValue<vk::DebugUtilsMessengerEXT> register_debug_callback(VulkanApplication *application, const vk::Instance &instance);

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
    };
} // namespace vl

#endif