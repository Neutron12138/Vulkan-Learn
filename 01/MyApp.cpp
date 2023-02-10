#ifndef MYAPP_CPP
#define MYAPP_CPP

#include "MyApp.hpp"

MyApp::MyApp()
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

MyApp::~MyApp()
{
    fout.close();
}

bool MyApp::CreateInstance()
{
    auto layer_result = vk::enumerateInstanceLayerProperties();
    if (layer_result.result != vk::Result::eSuccess)
    {
        ntl::log.loge(
            NTL_STRING("CreateInstance"),
            ntl::StringUtils::to_string(
                NTL_STRING("Failed to get layers, error code:"),
                static_cast<long>(layer_result.result)));
        return false;
    }

    auto fail = check_layer_support(VALIDATION_LAYERS, layer_result.value);
    if (fail.size() != 0)
    {
        ntl::log.loge(
            NTL_STRING("CreateInstance"),
            NTL_STRING("There are unsupported layers"));
        return false;
    }

    auto instance_result = create_instance("01", VALIDATION_LAYERS);

    if (instance_result.result != vk::Result::eSuccess)
    {
        ntl::log.loge(
            NTL_STRING("CreateInstance"),
            ntl::StringUtils::to_string(
                NTL_STRING("Failed to create an instance, error code:"),
                static_cast<long>(instance_result.result)));
        return false;
    }

    ntl::log.logi(
        NTL_STRING("CreateInstance"),
        NTL_STRING("Instance created successfully"));
    instance = instance_result.value;

    return true;
}

bool MyApp::SetupDebugMessenger()
{
    auto result = register_debug_callback(this, instance);

    if (result.result != vk::Result::eSuccess)
    {

        ntl::log.loge(
            NTL_STRING("CreateDebugMessenger"),
            ntl::StringUtils::to_string(
                NTL_STRING("Failed to create debug messenger, error code:"),
                static_cast<long>(result.result)));
        return false;
    }

    ntl::log.logi(
        NTL_STRING("CreateDebugMessenger"),
        NTL_STRING("Debug Messenger created successfully"));
    messenger = result.value;

    return true;
}

bool MyApp::PickPhysicalDevice()
{
    vk::PhysicalDevice result =
        pick_suitable_physical_device(
            instance,
            queue_family,
            is_physical_device_suitable<vl::DefaultQueueFamilyIndices>);
    if (result == vk::PhysicalDevice(nullptr))
    {
        ntl::log.loge(
            NTL_STRING("PickPhysicalDevice"),
            NTL_STRING("Unable to find suitable physical device"));
        return false;
    }

    ntl::log.logi(
        NTL_STRING("PickPhysicalDevice"),
        NTL_STRING("Successfully found the suitable physical device"));
    ntl::log.logi(
        NTL_STRING("PickPhysicalDevice"),
        queue_family.format());
    device = result;

    return true;
}

bool MyApp::CreateDevice()
{
    return true;
}

void MyApp::onDestroyed()
{
    destroy_debug_callback(messenger, instance);
    instance.destroy();

    ntl::log.logi(
        NTL_STRING("onDestroyed"),
        NTL_STRING("All objects are cleaned up"));
}

void MyApp::onDisplay()
{
}

#endif
