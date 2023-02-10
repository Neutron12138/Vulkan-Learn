#ifndef MYAPP_HPP
#define MYAPP_HPP

#include <iostream>
#include <optional>
#include "../src/VL.hpp"
#include "../src/VL.cpp"

class MyApp : public vl::VulkanApplication
{
private:
    const glm::uvec2 WINDOW_SIZE = glm::uvec2(640, 480);
    const std::vector<std::string> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
    ntl::OutputFileStream fout;
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT messenger;
    vk::PhysicalDevice device = nullptr;
    vl::DefaultQueueFamilyIndices queue_family;

public:
    MyApp();
    ~MyApp() override;

public:
    bool CreateInstance() override;
    bool SetupDebugMessenger() override;
    bool PickPhysicalDevice() override;
    bool CreateDevice() override;
    void onDestroyed() override;
    void onDisplay() override;
};

#endif
