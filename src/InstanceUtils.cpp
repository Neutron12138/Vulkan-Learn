#ifndef __VL_INSTANCEUTILS_CPP__
#define __VL_INSTANCEUTILS_CPP__

#include <SFML/Graphics.hpp>
#include "InstanceUtils.hpp"

namespace vl
{
    vk::ResultValue<vk::Instance>
    InstanceUtils::create_instance(
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
    InstanceUtils::check_layer_support(
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
} // namespace vl

#endif