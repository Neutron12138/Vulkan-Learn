#ifndef __VL_VULKANAPPLICATION_CPP__
#define __VL_VULKANAPPLICATION_CPP__

#include <SFML/Graphics.hpp>
#include "VulkanApplication.hpp"

namespace vl
{
    VkBool32
    VulkanApplication::debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData)
    {
        ntl::log.logu(
            L"",
            ntl::StringUtils::to_string(
                messageSeverity,
                L",",
                messageTypes,
                L",",
                pCallbackData));
        return VK_FALSE;
    }

    int
    VulkanApplication::run()
    {
        m_is_running = true;
        onCreated();

        onIdle();
        m_last_idle = ntl::get_current_time();

        while (m_is_running && m_window.is_open())
        {
            m_window.process_events();

            ntl::Time current_time = ntl::get_current_time();
            m_delta_time = current_time - m_last_idle;
            onIdle();
            m_last_idle = current_time;
        }

        onDestroyed();
        return m_exit_code;
    }

    void
    VulkanApplication::onIdle()
    {
        onDisplay();
        m_window.display();
    }

    void VulkanApplication::onCreated()
    {
        if (!CreateInstance())
            quit(EXIT_FAILURE);
        if (!CreateDebugMessenger())
            quit(EXIT_FAILURE);
    }

    vk::ResultValue<vk::Instance>
    VulkanApplication::create_instance(
        const std::string &name,
        const std::vector<std::string> &validation_layers)
    {
        // 应用信息
        vk::ApplicationInfo app_info;
        app_info.setPApplicationName(name.data());
        app_info.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
        app_info.setPEngineName("No Engine");
        app_info.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));
        app_info.setApiVersion(VK_API_VERSION_1_0);

        // 拓展
        std::vector<const char *> extensions = sf::Vulkan::getGraphicsRequiredInstanceExtensions();
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        // 实例创建信息
        vk::InstanceCreateInfo create_info;
        create_info.setPApplicationInfo(&app_info);
        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = &extensions.at(0);
        create_info.enabledLayerCount = validation_layers.size();
        create_info.ppEnabledLayerNames = reinterpret_cast<const char *const *>(validation_layers.data());

        return vk::createInstance(create_info);
    }

    std::vector<std::string>
    VulkanApplication::check_layer_support(
        const std::vector<std::string> &validation_layers,
        const std::vector<vk::LayerProperties> &layers)
    {
        // 不支持的层
        std::vector<std::string> fail;

        for (auto v_iter = validation_layers.cbegin(); v_iter != validation_layers.cend(); v_iter++)
        {
            // 是否成功找到
            bool found = false;

            for (auto l_iter = layers.cbegin(); l_iter != layers.cend(); l_iter++)
            {
                if (l_iter->layerName.data() == *v_iter)
                    found = true;
            }

            if (!found)
                fail.push_back(*v_iter);
        }

        return fail;
    }

    vk::ResultValue<vk::DebugUtilsMessengerEXT>
    VulkanApplication::register_debug_callback(
        VulkanApplication *application,
        const vk::Instance &instance)
    {
        vk::DebugUtilsMessengerCreateInfoEXT create_info;
        create_info.setMessageSeverity(
            vk::DebugUtilsMessageSeverityFlagsEXT(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError));
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
                L"VulkanApplication::register_debug_callback",
                L"Unable to get function \"vkCreateDebugUtilsMessengerEXT\"");
            result = vk::Result::eErrorExtensionNotPresent;
        }

        return vk::ResultValue<vk::DebugUtilsMessengerEXT>(
            result,
            vk::DebugUtilsMessengerEXT(messenger));
    }

    VkBool32 VulkanApplication::default_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {
        VulkanApplication *application = static_cast<VulkanApplication *>(pUserData);
        if (application != nullptr)
            return application->debug_callback(messageSeverity, messageTypes, pCallbackData);
        else
            return VK_FALSE;
    }

} // namespace vl

#endif