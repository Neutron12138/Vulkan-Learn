#ifndef MYAPP_HPP
#define MYAPP_HPP

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
    vk::PhysicalDevice device = nullptr;

public:
    MyApp();
    ~MyApp() override;

public:
    bool CreateInstance() override;
    bool SetupDebugMessenger() override;
    bool PickPhysicalDevice() override;
    void onDestroyed() override;
    void onDisplay() override;
};

#endif
