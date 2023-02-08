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

bool MyApp::SetupDebugMessenger()
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

bool MyApp::PickPhysicalDevice()
{
    vk::PhysicalDevice result = pick_suitable_physical_device(instance, is_physical_device_suitable);
    if (result == vk::PhysicalDevice(nullptr))
    {
        ntl::log.loge(
            L"PickPhysicalDevice",
            L"Unable to find suitable physical device");
        return false;
    }

    ntl::log.logi(
        L"PickPhysicalDevice",
        L"Successfully found the appropriate physical device");
    device = result;

    return true;
}

void MyApp::onDestroyed()
{
    destroy_debug_callback(messenger, instance);
    instance.destroy();

    ntl::log.logi(
        L"onDestroyed",
        L"All objects are cleaned up");
}

void MyApp::onDisplay()
{
}

#endif
