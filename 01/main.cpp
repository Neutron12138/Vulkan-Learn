#include <iostream>
#include "../src/VL.hpp"
#include "../src/VL.cpp"

class MyApp : public vl::VulkanApplication
{
private:
    const glm::uvec2 WINDOW_SIZE = glm::uvec2(640, 480);
    const std::vector<std::string> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
    std::wofstream fout;
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT messenger;

public:
    MyApp()
    {
        fout.open("log.txt");
        if (fout.fail())
        {
            std::cout << "" << std::endl;
            exit(EXIT_SUCCESS);
        }
        ntl::log.set_output(&fout);

        m_window.create(
            sf::VideoMode(WINDOW_SIZE.x, WINDOW_SIZE.y),
            "01",
            sf::Style::Close,
            sf::ContextSettings());
    }

    ~MyApp() override
    {
        fout.close();
    }

public:
    bool CreateInstance() override
    {
        auto layer_result = vk::enumerateInstanceLayerProperties();
        if (layer_result.result != vk::Result::eSuccess)
        {
            ntl::log.loge(
                L"CreateInstance",
                ntl::StringUtils::to_string(
                    L"Failed to get layers, error code:",
                    static_cast<long>(layer_result.result)));
            return false;
        }

        auto fail = check_layer_support(VALIDATION_LAYERS, layer_result.value);
        if (fail.size() != 0)
        {
            ntl::log.loge(
                L"CreateInstance",
                L"There are unsupported layers");
            return false;
        }

        auto instance_result = create_instance("01", VALIDATION_LAYERS);

        if (instance_result.result != vk::Result::eSuccess)
        {
            ntl::log.loge(
                L"CreateInstance",
                ntl::StringUtils::to_string(
                    L"Failed to create an instance, error code:",
                    static_cast<long>(instance_result.result)));
            return false;
        }

        ntl::log.logi(
            L"CreateInstance",
            L"Instance created successfully");
        instance = instance_result.value;

        return true;
    }

    bool CreateDebugMessenger() override
    {
        auto result = register_debug_callback(this, instance);

        if (result.result != vk::Result::eSuccess)
        {

            ntl::log.loge(
                L"CreateDebugMessenger",
                ntl::StringUtils::to_string(
                    L"Failed to create debug messenger, error code:",
                    static_cast<long>(result.result)));
            return false;
        }

        ntl::log.logi(
            L"CreateDebugMessenger",
            L"Debug Messenger created successfully");
        messenger = result.value;

        return true;
    }

    void onDestroyed() override
    {
        auto DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (DestroyDebugUtilsMessengerEXT != nullptr)
            DestroyDebugUtilsMessengerEXT(instance, VkDebugUtilsMessengerEXT(messenger), nullptr);
        else
            ntl::log.loge(
                L"onDestroyed",
                L"Unable to get function \"vkDestroyDebugUtilsMessengerEXT\"");

        instance.destroy();

        ntl::log.logi(
            L"onDestroyed",
            L"All objects are cleaned up");
    }

    void onDisplay() override
    {
    }
};

int main()
{
    MyApp app;
    return app.run();
}