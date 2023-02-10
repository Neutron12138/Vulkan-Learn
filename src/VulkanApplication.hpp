#ifndef __VL_VULKANAPPLICATION_HPP__
#define __VL_VULKANAPPLICATION_HPP__

#include <ntl/NTL.hpp>
#include <nwl/NWL.hpp>
#include "VulkanUtils.hpp"

namespace vl
{
    /// @brief Vulkan程序
    class VulkanApplication : public ntl::Application, public VulkanUtils
    {
    public:
        using SelfType = VulkanApplication;
        using ParentType = ntl::Application;

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
        int run() override;
        void onIdle() override;
        void onCreated() override;

    public:
        /// @brief 当绘制时调用
        virtual void onDisplay() = 0;

        /// @brief 创建实例
        /// @return 是否成功
        virtual bool CreateInstance() = 0;

        /// @brief 创建调试信息回调
        /// @return 是否成功
        virtual bool SetupDebugMessenger() = 0;

        /// @brief 选择物理设备
        /// @return 是否成功
        virtual bool PickPhysicalDevice() = 0;

        /// @brief 创建逻辑设备
        /// @return 是否成功
        virtual bool CreateDevice() = 0;
    };
} // namespace vl

#endif