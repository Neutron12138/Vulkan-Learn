/*
 * Copyright (c) 2015-2019 The Khronos Group Inc.
 * Copyright (c) 2015-2019 Valve Corporation
 * Copyright (c) 2015-2019 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Jeremy Hayes <jeremy@lunarg.com>
 * Author: Charles Giessen <charles@lunarg.com>
 */

#ifdef UNICODE
#undef UNICODE
#endif
#define VK_USE_PLATFORM_METAL_EXT
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xutil.h>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <linux/input.h>
#include "xdg-shell-client-header.h"
#include "xdg-decoration-client-header.h"
#endif

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>

#include <sstream>
#include <iostream>
#include <memory>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_TYPESAFE_CONVERSION
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

#include "linmath.h"

#ifndef NDEBUG
#define VERIFY(x) assert(x)
#else
#define VERIFY(x) ((void)(x))
#endif

#define APP_SHORT_NAME "vkcubepp"

// Allow a maximum of two outstanding presentation operations.
constexpr uint32_t FRAME_LAG = 2;

#ifdef _WIN32
#define ERR_EXIT(err_msg, err_class)                        \
    do                                                      \
    {                                                       \
        if (!suppress_popups)                               \
            MessageBox(nullptr, err_msg, err_class, MB_OK); \
        exit(1);                                            \
    } while (0)
#endif

// easier to use the C function for extension functions
PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                              const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugUtilsMessengerEXT *pMessenger)
{
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                                           VkAllocationCallbacks const *pAllocator)
{
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

struct texture_object
{
    vk::Sampler sampler;

    vk::Image image;
    vk::Buffer buffer;
    vk::ImageLayout imageLayout{vk::ImageLayout::eUndefined};

    vk::MemoryAllocateInfo mem_alloc;
    vk::DeviceMemory mem;
    vk::ImageView view;

    uint32_t tex_width{0};
    uint32_t tex_height{0};
};

static char const *const tex_files[] = {"lunarg.ppm"};

static int validation_error = 0;

struct vktexcube_vs_uniform
{
    // Must start with MVP
    float mvp[4][4];
    float position[12 * 3][4];
    float attr[12 * 3][4];
};

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
// clang-format off
static const float g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Z side
     1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Y side
     1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // +Y side
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  // +X side
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // +Z side
    -1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = {
    0.0f, 1.0f,  // -X side
    1.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Z side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Y side
    1.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // +Y side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 0.0f,  // +X side
    0.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // +Z side
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
};
// clang-format on

struct SwapchainImageResources
{
    vk::Image image;
    vk::CommandBuffer cmd;
    vk::CommandBuffer graphics_to_present_cmd;
    vk::ImageView view;
    vk::Buffer uniform_buffer;
    vk::DeviceMemory uniform_memory;
    void *uniform_memory_ptr = nullptr;
    vk::Framebuffer framebuffer;
    vk::DescriptorSet descriptor_set;
};

struct Demo
{
    void build_image_ownership_cmd(const SwapchainImageResources &swapchain_image_resource);
    vk::Bool32 check_layers(const std::vector<const char *> &check_names, const std::vector<vk::LayerProperties> &layers);
    void cleanup();
    void destroy_swapchain_related_resources();
    void create_device();
    void destroy_texture(texture_object &tex_objs);
    void draw();
    void draw_build_cmd(const SwapchainImageResources &swapchain_image_resource);
    void prepare_init_cmd();
    void flush_init_cmd();
    void init(int argc, char **argv);
    void init_connection();
    void init_vk();
    void init_vk_swapchain();
    void prepare();
    void prepare_buffers();
    void prepare_cube_data_buffers();
    void prepare_depth();
    void prepare_descriptor_layout();
    void prepare_descriptor_pool();
    void prepare_descriptor_set();
    void prepare_framebuffers();
    vk::ShaderModule prepare_shader_module(const uint32_t *code, size_t size);
    vk::ShaderModule prepare_vs();
    vk::ShaderModule prepare_fs();
    void prepare_pipeline();
    void prepare_render_pass();
    void prepare_texture_image(const char *filename, texture_object &tex_obj, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                               vk::MemoryPropertyFlags required_props);
    void prepare_texture_buffer(const char *filename, texture_object &tex_obj);
    void prepare_textures();

    void resize();
    void create_surface();
    void set_image_layout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                          vk::AccessFlags srcAccessMask, vk::PipelineStageFlags src_stages, vk::PipelineStageFlags dest_stages);
    void update_data_buffer();
    bool loadTexture(const char *filename, uint8_t *rgba_data, vk::SubresourceLayout &layout, uint32_t &width, uint32_t &height);
    bool memory_type_from_properties(uint32_t typeBits, vk::MemoryPropertyFlags requirements_mask, uint32_t &typeIndex);
    vk::SurfaceFormatKHR pick_surface_format(const std::vector<vk::SurfaceFormatKHR> &surface_formats);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                   VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                                   void *pUserData);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    void run();
    void create_window();
#endif

    std::string name = "vkcubepp"; // Name to put on the window/icon
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    HINSTANCE connection = nullptr; // hInstance - Windows Instance
    HWND window = nullptr;          // hWnd - window handle
    POINT minsize = {0, 0};         // minimum window size
#endif

    vk::SurfaceKHR surface;
    bool prepared = false;
    bool use_staging_buffer = false;
    bool use_xlib = false;
    bool separate_present_queue = false;
    int32_t gpu_number = 0;

    vk::Instance inst;
    vk::DebugUtilsMessengerEXT debug_messenger;
    vk::PhysicalDevice gpu;
    vk::Device device;
    vk::Queue graphics_queue;
    vk::Queue present_queue;
    uint32_t graphics_queue_family_index = 0;
    uint32_t present_queue_family_index = 0;
    std::array<vk::Semaphore, FRAME_LAG> image_acquired_semaphores;
    std::array<vk::Semaphore, FRAME_LAG> draw_complete_semaphores;
    std::array<vk::Semaphore, FRAME_LAG> image_ownership_semaphores;
    vk::PhysicalDeviceProperties gpu_props;
    std::vector<vk::QueueFamilyProperties> queue_props;
    vk::PhysicalDeviceMemoryProperties memory_properties;

    std::vector<const char *> enabled_instance_extensions;
    std::vector<const char *> enabled_layers;
    std::vector<const char *> enabled_device_extensions;

    uint32_t width = 0;
    uint32_t height = 0;
    vk::Format format;
    vk::ColorSpaceKHR color_space;

    vk::SwapchainKHR swapchain;
    std::vector<SwapchainImageResources> swapchain_image_resources;
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    std::array<vk::Fence, FRAME_LAG> fences;
    uint32_t frame_index = 0;

    vk::CommandPool cmd_pool;
    vk::CommandPool present_cmd_pool;

    struct
    {
        vk::Format format;
        vk::Image image;
        vk::MemoryAllocateInfo mem_alloc;
        vk::DeviceMemory mem;
        vk::ImageView view;
    } depth;

    static int32_t const texture_count = 1;
    std::array<texture_object, texture_count> textures;
    texture_object staging_texture;

    struct
    {
        vk::Buffer buf;
        vk::MemoryAllocateInfo mem_alloc;
        vk::DeviceMemory mem;
        vk::DescriptorBufferInfo buffer_info;
    } uniform_data;

    vk::CommandBuffer cmd; // Buffer for initialization commands
    vk::PipelineLayout pipeline_layout;
    vk::DescriptorSetLayout desc_layout;
    vk::PipelineCache pipelineCache;
    vk::RenderPass render_pass;
    vk::Pipeline pipeline;

    mat4x4 projection_matrix = {};
    mat4x4 view_matrix = {};
    mat4x4 model_matrix = {};

    float spin_angle = 0.0f;
    float spin_increment = 0.0f;
    bool pause = false;

    vk::ShaderModule vert_shader_module;
    vk::ShaderModule frag_shader_module;

    vk::DescriptorPool desc_pool;
    vk::DescriptorSet desc_set;

    std::vector<vk::Framebuffer> framebuffers;

    bool quit = false;
    uint32_t curFrame = 0;
    uint32_t frameCount = 0;
    bool validate = false;
    bool in_callback = false;
    bool use_debug_messenger = false;
    bool use_break = false;
    bool suppress_popups = false;
    bool force_errors = false;

    uint32_t current_buffer = 0;
};

#ifdef _WIN32
// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

void Demo::build_image_ownership_cmd(const SwapchainImageResources &swapchain_image_resource)
{
    auto result = swapchain_image_resource.graphics_to_present_cmd.begin(
        vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
    VERIFY(result == vk::Result::eSuccess);

    auto const image_ownership_barrier =
        vk::ImageMemoryBarrier()
            .setSrcAccessMask(vk::AccessFlags())
            .setDstAccessMask(vk::AccessFlags())
            .setOldLayout(vk::ImageLayout::ePresentSrcKHR)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
            .setSrcQueueFamilyIndex(graphics_queue_family_index)
            .setDstQueueFamilyIndex(present_queue_family_index)
            .setImage(swapchain_image_resource.image)
            .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

    swapchain_image_resource.graphics_to_present_cmd.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe,
                                                                     vk::PipelineStageFlagBits::eBottomOfPipe,
                                                                     vk::DependencyFlagBits(), {}, {}, image_ownership_barrier);

    result = swapchain_image_resource.graphics_to_present_cmd.end();
    VERIFY(result == vk::Result::eSuccess);
}

vk::Bool32 Demo::check_layers(const std::vector<const char *> &check_names, const std::vector<vk::LayerProperties> &layers)
{
    for (const auto &name : check_names)
    {
        vk::Bool32 found = VK_FALSE;
        for (const auto &layer : layers)
        {
            if (!strcmp(name, layer.layerName))
            {
                found = VK_TRUE;
                break;
            }
        }
        if (!found)
        {
            fprintf(stderr, "Cannot find layer: %s\n", name);
            return 0;
        }
    }
    return VK_TRUE;
}

void Demo::cleanup()
{
    prepared = false;
    auto result = device.waitIdle();
    VERIFY(result == vk::Result::eSuccess);

    destroy_swapchain_related_resources();
    // Wait for fences from present operations
    for (uint32_t i = 0; i < FRAME_LAG; i++)
    {
        device.destroyFence(fences[i]);
        device.destroySemaphore(image_acquired_semaphores[i]);
        device.destroySemaphore(draw_complete_semaphores[i]);
        if (separate_present_queue)
        {
            device.destroySemaphore(image_ownership_semaphores[i]);
        }
    }

    device.destroySwapchainKHR(swapchain);

    device.destroy();
    inst.destroySurfaceKHR(surface);

#if defined(VK_USE_PLATFORM_XLIB_KHR)
    XDestroyWindow(display, xlib_window);
    XCloseDisplay(display);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    xcb_destroy_window(connection, xcb_window);
    xcb_disconnect(connection);
    free(atom_wm_delete_window);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    if (keyboard)
        wl_keyboard_destroy(keyboard);
    if (pointer)
        wl_pointer_destroy(pointer);
    if (seat)
        wl_seat_destroy(seat);
    xdg_toplevel_destroy(window_toplevel);
    xdg_surface_destroy(window_surface);
    wl_surface_destroy(window);
    xdg_wm_base_destroy(wm_base);
    if (xdg_decoration_mgr)
    {
        zxdg_toplevel_decoration_v1_destroy(toplevel_decoration);
        zxdg_decoration_manager_v1_destroy(xdg_decoration_mgr);
    }
    wl_compositor_destroy(compositor);
    wl_registry_destroy(registry);
    wl_display_disconnect(display);
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
    event_buffer->Release(event_buffer);
    window->Release(window);
    dfb->Release(dfb);
#endif
    if (use_debug_messenger)
    {
        inst.destroyDebugUtilsMessengerEXT(debug_messenger);
    }
    inst.destroy();
}

void Demo::create_device()
{
    float priorities = 0.0;

    std::vector<vk::DeviceQueueCreateInfo> queues;
    queues.push_back(vk::DeviceQueueCreateInfo().setQueueFamilyIndex(graphics_queue_family_index).setQueuePriorities(priorities));

    if (separate_present_queue)
    {
        queues.push_back(
            vk::DeviceQueueCreateInfo().setQueueFamilyIndex(present_queue_family_index).setQueuePriorities(priorities));
    }

    auto deviceInfo = vk::DeviceCreateInfo().setQueueCreateInfos(queues).setPEnabledExtensionNames(enabled_device_extensions);
    auto device_return = gpu.createDevice(deviceInfo);
    VERIFY(device_return.result == vk::Result::eSuccess);
    device = device_return.value;
}

void Demo::destroy_texture(texture_object &tex_objs)
{
    // clean up staging resources
    device.freeMemory(tex_objs.mem);
    if (tex_objs.image)
        device.destroyImage(tex_objs.image);
    if (tex_objs.buffer)
        device.destroyBuffer(tex_objs.buffer);
}

void Demo::draw()
{
    // Ensure no more than FRAME_LAG renderings are outstanding
    device.waitForFences(fences[frame_index], VK_TRUE, UINT64_MAX);
    device.resetFences({fences[frame_index]});

    vk::Result acquire_result;
    do
    {
        acquire_result =
            device.acquireNextImageKHR(swapchain, UINT64_MAX, image_acquired_semaphores[frame_index], vk::Fence(), &current_buffer);
        if (acquire_result == vk::Result::eErrorOutOfDateKHR)
        {
            // demo.swapchain is out of date (e.g. the window was resized) and
            // must be recreated:
            resize();
        }
        else if (acquire_result == vk::Result::eSuboptimalKHR)
        {
            // swapchain is not as optimal as it could be, but the platform's
            // presentation engine will still present the image correctly.
            break;
        }
        else if (acquire_result == vk::Result::eErrorSurfaceLostKHR)
        {
            inst.destroySurfaceKHR(surface);
            create_surface();
            resize();
        }
        else
        {
            VERIFY(acquire_result == vk::Result::eSuccess);
        }
    } while (acquire_result != vk::Result::eSuccess);

    update_data_buffer();

    // Wait for the image acquired semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.
    vk::PipelineStageFlags const pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    auto submit_result = graphics_queue.submit(vk::SubmitInfo()
                                                   .setWaitDstStageMask(pipe_stage_flags)
                                                   .setWaitSemaphores(image_acquired_semaphores[frame_index])
                                                   .setCommandBuffers(swapchain_image_resources[current_buffer].cmd)
                                                   .setSignalSemaphores(draw_complete_semaphores[frame_index]),
                                               fences[frame_index]);
    VERIFY(submit_result == vk::Result::eSuccess);

    if (separate_present_queue)
    {
        // If we are using separate queues, change image ownership to the
        // present queue before presenting, waiting for the draw complete
        // semaphore and signalling the ownership released semaphore when
        // finished
        auto change_owner_result =
            present_queue.submit(vk::SubmitInfo()
                                     .setWaitDstStageMask(pipe_stage_flags)
                                     .setWaitSemaphores(draw_complete_semaphores[frame_index])
                                     .setCommandBuffers(swapchain_image_resources[current_buffer].graphics_to_present_cmd)
                                     .setSignalSemaphores(image_ownership_semaphores[frame_index]));
        VERIFY(change_owner_result == vk::Result::eSuccess);
    }

    // If we are using separate queues we have to wait for image ownership,
    // otherwise wait for draw complete
    auto present_result =
        present_queue.presentKHR(vk::PresentInfoKHR()
                                     .setWaitSemaphores(separate_present_queue ? image_ownership_semaphores[frame_index]
                                                                               : draw_complete_semaphores[frame_index])
                                     .setSwapchains(swapchain)
                                     .setImageIndices(current_buffer));
    frame_index += 1;
    frame_index %= FRAME_LAG;
    if (present_result == vk::Result::eErrorOutOfDateKHR)
    {
        // swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        resize();
    }
    else if (present_result == vk::Result::eSuboptimalKHR)
    {
        // SUBOPTIMAL could be due to resize
        vk::SurfaceCapabilitiesKHR surfCapabilities;
        auto caps_result = gpu.getSurfaceCapabilitiesKHR(surface, &surfCapabilities);
        VERIFY(caps_result == vk::Result::eSuccess);
        if (surfCapabilities.currentExtent.width != width || surfCapabilities.currentExtent.height != height)
        {
            resize();
        }
    }
    else if (present_result == vk::Result::eErrorSurfaceLostKHR)
    {
        inst.destroySurfaceKHR(surface);
        create_surface();
        resize();
    }
    else
    {
        VERIFY(present_result == vk::Result::eSuccess);
    }
}

void Demo::draw_build_cmd(const SwapchainImageResources &swapchain_image_resource)
{
    const auto commandBuffer = swapchain_image_resource.cmd;
    vk::ClearValue const clearValues[2] = {vk::ClearColorValue(std::array<float, 4>({{0.2f, 0.2f, 0.2f, 0.2f}})),
                                           vk::ClearDepthStencilValue(1.0f, 0u)};

    auto result = commandBuffer.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
    VERIFY(result == vk::Result::eSuccess);

    commandBuffer.beginRenderPass(vk::RenderPassBeginInfo()
                                      .setRenderPass(render_pass)
                                      .setFramebuffer(swapchain_image_resource.framebuffer)
                                      .setRenderArea(vk::Rect2D(vk::Offset2D{}, vk::Extent2D(width, height)))
                                      .setClearValueCount(2)
                                      .setPClearValues(clearValues),
                                  vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, swapchain_image_resource.descriptor_set,
                                     {});
    float viewport_dimension;
    float viewport_x = 0.0f;
    float viewport_y = 0.0f;
    if (width < height)
    {
        viewport_dimension = static_cast<float>(width);
        viewport_y = (height - width) / 2.0f;
    }
    else
    {
        viewport_dimension = static_cast<float>(height);
        viewport_x = (width - height) / 2.0f;
    }

    commandBuffer.setViewport(0, vk::Viewport()
                                     .setX(viewport_x)
                                     .setY(viewport_y)
                                     .setWidth(viewport_dimension)
                                     .setHeight(viewport_dimension)
                                     .setMinDepth(0.0f)
                                     .setMaxDepth(1.0f));

    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D{}, vk::Extent2D(width, height)));
    commandBuffer.draw(12 * 3, 1, 0, 0);
    // Note that ending the renderpass changes the image's layout from
    // COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
    commandBuffer.endRenderPass();

    if (separate_present_queue)
    {
        // We have to transfer ownership from the graphics queue family to
        // the
        // present queue family to be able to present.  Note that we don't
        // have
        // to transfer from present queue family back to graphics queue
        // family at
        // the start of the next frame because we don't care about the
        // image's
        // contents at that point.
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlagBits(), {}, {},
            vk::ImageMemoryBarrier()
                .setSrcAccessMask(vk::AccessFlags())
                .setDstAccessMask(vk::AccessFlags())
                .setOldLayout(vk::ImageLayout::ePresentSrcKHR)
                .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
                .setSrcQueueFamilyIndex(graphics_queue_family_index)
                .setDstQueueFamilyIndex(present_queue_family_index)
                .setImage(swapchain_image_resource.image)
                .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
    }

    result = commandBuffer.end();
    VERIFY(result == vk::Result::eSuccess);
}

void Demo::prepare_init_cmd()
{
    auto cmd_pool_return = device.createCommandPool(vk::CommandPoolCreateInfo().setQueueFamilyIndex(graphics_queue_family_index));
    VERIFY(cmd_pool_return.result == vk::Result::eSuccess);
    cmd_pool = cmd_pool_return.value;

    auto cmd_return = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
                                                        .setCommandPool(cmd_pool)
                                                        .setLevel(vk::CommandBufferLevel::ePrimary)
                                                        .setCommandBufferCount(1));
    VERIFY(cmd_return.result == vk::Result::eSuccess);
    cmd = cmd_return.value[0];

    auto result = cmd.begin(vk::CommandBufferBeginInfo());
    VERIFY(result == vk::Result::eSuccess);
}

void Demo::flush_init_cmd()
{
    auto result = cmd.end();
    VERIFY(result == vk::Result::eSuccess);

    auto fenceInfo = vk::FenceCreateInfo();
    if (force_errors)
    {
        // Remove sType to intentionally force validation layer errors.
        fenceInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    }
    auto fence_return = device.createFence(fenceInfo);
    VERIFY(fence_return.result == vk::Result::eSuccess);
    auto fence = fence_return.value;

    result = graphics_queue.submit(vk::SubmitInfo().setCommandBuffers(cmd), fence);
    VERIFY(result == vk::Result::eSuccess);

    result = device.waitForFences(fence, VK_TRUE, UINT64_MAX);
    VERIFY(result == vk::Result::eSuccess);

    device.freeCommandBuffers(cmd_pool, cmd);
    device.destroyFence(fence);
}

void Demo::init(int argc, char **argv)
{
    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};

    presentMode = vk::PresentModeKHR::eFifo;
    frameCount = UINT32_MAX;
    width = 500;
    height = 500;
    use_xlib = false;
    /* Autodetect suitable / best GPU by default */
    gpu_number = -1;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--use_staging") == 0)
        {
            use_staging_buffer = true;
            continue;
        }
        if ((strcmp(argv[i], "--present_mode") == 0) && (i < argc - 1))
        {
            presentMode = static_cast<vk::PresentModeKHR>(atoi(argv[i + 1]));
            i++;
            continue;
        }
        if (strcmp(argv[i], "--break") == 0)
        {
            use_break = true;
            continue;
        }
        if (strcmp(argv[i], "--validate") == 0)
        {
            validate = true;
            continue;
        }
        if (strcmp(argv[i], "--xlib") == 0)
        {
            fprintf(stderr, "--xlib is deprecated and no longer does anything");
            continue;
        }
        if (strcmp(argv[i], "--c") == 0 && frameCount == UINT32_MAX && i < argc - 1 &&
            sscanf(argv[i + 1], "%" SCNu32, &frameCount) == 1)
        {
            i++;
            continue;
        }
        if (strcmp(argv[i], "--width") == 0 && i < argc - 1 && sscanf(argv[i + 1], "%" SCNu32, &width) == 1)
        {
            i++;
            continue;
        }
        if (strcmp(argv[i], "--height") == 0 && i < argc - 1 && sscanf(argv[i + 1], "%" SCNu32, &height) == 1)
        {
            i++;
            continue;
        }
        if (strcmp(argv[i], "--suppress_popups") == 0)
        {
            suppress_popups = true;
            continue;
        }
        if ((strcmp(argv[i], "--gpu_number") == 0) && (i < argc - 1))
        {
            gpu_number = atoi(argv[i + 1]);
            assert(gpu_number >= 0);
            i++;
            continue;
        }
        if (strcmp(argv[i], "--force_errors") == 0)
        {
            force_errors = true;
            continue;
        }
        std::stringstream usage;
        usage << "Usage:\n  " << APP_SHORT_NAME << "\t[--use_staging] [--validate]\n"
              << "\t[--break] [--c <framecount>] [--suppress_popups]\n"
              << "\t[--gpu_number <index of physical device>]\n"
              << "\t[--present_mode <present mode enum>]\n"
              << "\t[--width <width>] [--height <height>]\n"
              << "\t[--force_errors]\n"
              << "\t<present_mode_enum>\n"
              << "\t\tVK_PRESENT_MODE_IMMEDIATE_KHR = " << VK_PRESENT_MODE_IMMEDIATE_KHR << "\n"
              << "\t\tVK_PRESENT_MODE_MAILBOX_KHR = " << VK_PRESENT_MODE_MAILBOX_KHR << "\n"
              << "\t\tVK_PRESENT_MODE_FIFO_KHR = " << VK_PRESENT_MODE_FIFO_KHR << "\n"
              << "\t\tVK_PRESENT_MODE_FIFO_RELAXED_KHR = " << VK_PRESENT_MODE_FIFO_RELAXED_KHR << "\n";

#if defined(_WIN32)
        if (!suppress_popups)
            MessageBox(nullptr, usage.str().c_str(), "Usage Error", MB_OK);
#else
        std::cerr << usage.str();
        std::cerr.flush();
#endif
        exit(1);
    }

    if (!use_xlib)
    {
        init_connection();
    }

    init_vk();

    spin_angle = 4.0f;
    spin_increment = 0.2f;
    pause = false;

    mat4x4_perspective(projection_matrix, static_cast<float>(degreesToRadians(45.0f)), 1.0f, 0.1f, 100.0f);
    mat4x4_look_at(view_matrix, eye, origin, up);
    mat4x4_identity(model_matrix);

    projection_matrix[1][1] *= -1; // Flip projection matrix from GL to Vulkan orientation.
}

void Demo::init_connection()
{
}

VKAPI_ATTR VkBool32 VKAPI_CALL Demo::debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                              VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                              void *pUserData)
{
    std::ostringstream message;
    Demo &demo = *static_cast<Demo *>(pUserData);

    if (demo.use_break)
    {
#ifndef WIN32
        raise(SIGTRAP);
#else
        DebugBreak();
#endif
    }
    message << vk::to_string(vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity));
    message << " : " + vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(messageType));

    if (vk::DebugUtilsMessageTypeFlagsEXT(messageType) & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
    {
        validation_error = 1;
    }

    message << " - Message Id Number: " << std::to_string(pCallbackData->messageIdNumber);
    message << " | Message Id Name: " << pCallbackData->pMessageIdName << "\n\t" << pCallbackData->pMessage << "\n";

    if (pCallbackData->objectCount > 0)
    {
        message << "\n\tObjects - " << pCallbackData->objectCount << "\n";
        for (uint32_t object = 0; object < pCallbackData->objectCount; ++object)
        {
            if (NULL != pCallbackData->pObjects[object].pObjectName && strlen(pCallbackData->pObjects[object].pObjectName) > 0)
            {
                message << "\t\tObject[" << object << "] - "
                        << vk::to_string(vk::ObjectType(pCallbackData->pObjects[object].objectType)) << ", Handle "
                        << pCallbackData->pObjects[object].objectHandle << ", Name \""
                        << pCallbackData->pObjects[object].pObjectName << "\"\n";
            }
            else
            {
                message << "\t\tObject[" << object << "] - "
                        << vk::to_string(vk::ObjectType(pCallbackData->pObjects[object].objectType)) << ", Handle "
                        << pCallbackData->pObjects[object].objectHandle << "\n";
            }
        }
    }
    if (pCallbackData->cmdBufLabelCount > 0)
    {
        message << "\n\tCommand Buffer Labels - " << pCallbackData->cmdBufLabelCount << "\n";
        for (uint32_t cmd_buf_label = 0; cmd_buf_label < pCallbackData->cmdBufLabelCount; ++cmd_buf_label)
        {
            message << "\t\tLabel[" << cmd_buf_label << "] - " << pCallbackData->pCmdBufLabels[cmd_buf_label].pLabelName << " { "
                    << pCallbackData->pCmdBufLabels[cmd_buf_label].color[0] << ", "
                    << pCallbackData->pCmdBufLabels[cmd_buf_label].color[1] << ", "
                    << pCallbackData->pCmdBufLabels[cmd_buf_label].color[2] << ", "
                    << pCallbackData->pCmdBufLabels[cmd_buf_label].color[2] << "}\n";
        }
    }

#ifdef _WIN32

    if (!demo.suppress_popups)
    {
        demo.in_callback = true;
        auto message_string = message.str();
        MessageBox(NULL, message_string.c_str(), "Alert", MB_OK);
        demo.in_callback = false;
    }
#endif
    return false; // Don't bail out, but keep going.
}

void Demo::init_vk()
{
    std::vector<char const *> instance_validation_layers = {"VK_LAYER_KHRONOS_validation"};

    // Look for validation layers
    vk::Bool32 validation_found = VK_FALSE;
    if (validate)
    {
        auto layers = vk::enumerateInstanceLayerProperties();
        VERIFY(layers.result == vk::Result::eSuccess);

        validation_found = check_layers(instance_validation_layers, layers.value);
        if (validation_found)
        {
            enabled_layers.push_back("VK_LAYER_KHRONOS_validation");
        }

        else
        {
            ERR_EXIT(
                "vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n"
                "Please look at the Getting Started guide for additional information.\n",
                "vkCreateInstance Failure");
        }
    }

    /* Look for instance extensions */
    vk::Bool32 surfaceExtFound = VK_FALSE;
    vk::Bool32 platformSurfaceExtFound = VK_FALSE;
    bool portabilityEnumerationActive = false;

    auto instance_extensions_return = vk::enumerateInstanceExtensionProperties();
    VERIFY(instance_extensions_return.result == vk::Result::eSuccess);

    for (const auto &extension : instance_extensions_return.value)
    {
        if (!strcmp(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, extension.extensionName))
        {
            enabled_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
        else if (!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extension.extensionName))
        {
            use_debug_messenger = true;
            enabled_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        else if (!strcmp(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, extension.extensionName))
        {
            // We want cube to be able to enumerate drivers that support the portability_subset extension, so we have to enable the
            // portability enumeration extension.
            portabilityEnumerationActive = true;
            enabled_instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        }
        else if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, extension.extensionName))
        {
            surfaceExtFound = 1;
            enabled_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        }
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        else if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, extension.extensionName))
        {
            platformSurfaceExtFound = 1;
            enabled_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        }
#endif
    }

    if (!surfaceExtFound)
    {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_SURFACE_EXTENSION_NAME
                 " extension.\n\n"
                 "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                 "Please look at the Getting Started guide for additional information.\n",
                 "vkCreateInstance Failure");
    }

    if (!platformSurfaceExtFound)
    {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
                 " extension.\n\n"
                 "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                 "Please look at the Getting Started guide for additional information.\n",
                 "vkCreateInstance Failure");
#endif
    }

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    auto debug_utils_create_info = vk::DebugUtilsMessengerCreateInfoEXT({}, severityFlags, messageTypeFlags,
                                                                        &debug_messenger_callback, static_cast<void *>(this));

    auto const app = vk::ApplicationInfo()
                         .setPApplicationName(APP_SHORT_NAME)
                         .setApplicationVersion(0)
                         .setPEngineName(APP_SHORT_NAME)
                         .setEngineVersion(0);
    auto const inst_info = vk::InstanceCreateInfo()
                               .setFlags(portabilityEnumerationActive ? vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR
                                                                      : static_cast<vk::InstanceCreateFlagBits>(0))
                               .setPNext((use_debug_messenger && validate) ? &debug_utils_create_info : nullptr)
                               .setPApplicationInfo(&app)
                               .setPEnabledLayerNames(enabled_layers)
                               .setPEnabledExtensionNames(enabled_instance_extensions);

    auto instance_result = vk::createInstance(inst_info);
    if (instance_result.result == vk::Result::eErrorIncompatibleDriver)
    {
        ERR_EXIT(
            "Cannot find a compatible Vulkan installable client driver (ICD).\n\n"
            "Please look at the Getting Started guide for additional information.\n",
            "vkCreateInstance Failure");
    }
    else if (instance_result.result == vk::Result::eErrorExtensionNotPresent)
    {
        ERR_EXIT(
            "Cannot find a specified extension library.\n"
            "Make sure your layers path is set appropriately.\n",
            "vkCreateInstance Failure");
    }
    else if (instance_result.result != vk::Result::eSuccess)
    {
        ERR_EXIT(
            "vkCreateInstance failed.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.\n",
            "vkCreateInstance Failure");
    }
    inst = instance_result.value;

    if (use_debug_messenger)
    {
        pfnVkCreateDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(inst.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
        pfnVkDestroyDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(inst.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
        VERIFY(pfnVkCreateDebugUtilsMessengerEXT != nullptr && pfnVkDestroyDebugUtilsMessengerEXT != nullptr);
        auto create_debug_messenger_return = inst.createDebugUtilsMessengerEXT(debug_utils_create_info);
        VERIFY(create_debug_messenger_return.result == vk::Result::eSuccess);
        debug_messenger = create_debug_messenger_return.value;
    }

    auto physical_device_return = inst.enumeratePhysicalDevices();
    VERIFY(physical_device_return.result == vk::Result::eSuccess);
    auto physical_devices = physical_device_return.value;

    if (physical_devices.size() <= 0)
    {
        ERR_EXIT(
            "vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
            "Please look at the Getting Started guide for additional information.\n",
            "vkEnumeratePhysicalDevices Failure");
    }

    if (gpu_number >= 0 && !(static_cast<uint32_t>(gpu_number) < physical_devices.size()))
    {
        fprintf(stderr, "GPU %d specified is not present, GPU count = %zu\n", gpu_number, physical_devices.size());
        ERR_EXIT("Specified GPU number is not present", "User Error");
    }
#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
    gpu_number = find_display_gpu(gpu_number, physical_devices);
    if (gpu_number < 0)
    {
        printf("Cannot find any display!\n");
        fflush(stdout);
        exit(1);
    }
#else
    /* Try to auto select most suitable device */
    if (gpu_number == -1)
    {
        constexpr uint32_t device_type_count = static_cast<uint32_t>(vk::PhysicalDeviceType::eCpu) + 1;
        std::array<uint32_t, device_type_count> count_device_type{};

        for (uint32_t i = 0; i < physical_devices.size(); i++)
        {
            const auto physicalDeviceProperties = physical_devices[i].getProperties();
            assert(physicalDeviceProperties.deviceType <= vk::PhysicalDeviceType::eCpu);
            count_device_type[static_cast<int>(physicalDeviceProperties.deviceType)]++;
        }

        std::array<vk::PhysicalDeviceType, device_type_count> const device_type_preference = {
            vk::PhysicalDeviceType::eDiscreteGpu, vk::PhysicalDeviceType::eIntegratedGpu, vk::PhysicalDeviceType::eVirtualGpu,
            vk::PhysicalDeviceType::eCpu, vk::PhysicalDeviceType::eOther};

        vk::PhysicalDeviceType search_for_device_type = vk::PhysicalDeviceType::eDiscreteGpu;
        for (uint32_t i = 0; i < sizeof(device_type_preference) / sizeof(vk::PhysicalDeviceType); i++)
        {
            if (count_device_type[static_cast<int>(device_type_preference[i])])
            {
                search_for_device_type = device_type_preference[i];
                break;
            }
        }

        for (uint32_t i = 0; i < physical_devices.size(); i++)
        {
            const auto physicalDeviceProperties = physical_devices[i].getProperties();
            if (physicalDeviceProperties.deviceType == search_for_device_type)
            {
                gpu_number = i;
                break;
            }
        }
    }
#endif
    assert(gpu_number >= 0);
    gpu = physical_devices[gpu_number];
    {
        auto physicalDeviceProperties = gpu.getProperties();
        fprintf(stderr, "Selected GPU %d: %s, type: %s\n", gpu_number, physicalDeviceProperties.deviceName.data(),
                to_string(physicalDeviceProperties.deviceType).c_str());
    }

    /* Look for device extensions */
    vk::Bool32 swapchainExtFound = VK_FALSE;

    auto device_extension_return = gpu.enumerateDeviceExtensionProperties();
    VERIFY(device_extension_return.result == vk::Result::eSuccess);

    for (const auto &extension : device_extension_return.value)
    {
        if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, extension.extensionName))
        {
            swapchainExtFound = 1;
            enabled_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }
        else if (!strcmp("VK_KHR_portability_subset", extension.extensionName))
        {
            enabled_device_extensions.push_back("VK_KHR_portability_subset");
        }
    }

    if (!swapchainExtFound)
    {
        ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
                 " extension.\n\n"
                 "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
                 "Please look at the Getting Started guide for additional information.\n",
                 "vkCreateInstance Failure");
    }

    gpu.getProperties(&gpu_props);

    /* Call with nullptr data to get count */
    queue_props = gpu.getQueueFamilyProperties();
    assert(queue_props.size() >= 1);

    // Query fine-grained feature support for this device.
    //  If app has specific feature requirements it should check supported
    //  features based on this query
    vk::PhysicalDeviceFeatures physDevFeatures;
    gpu.getFeatures(&physDevFeatures);
}

void Demo::create_surface()
{
// Create a WSI surface for the window:
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    {
        auto const createInfo = vk::Win32SurfaceCreateInfoKHR().setHinstance(connection).setHwnd(window);

        auto result = inst.createWin32SurfaceKHR(&createInfo, nullptr, &surface);
        VERIFY(result == vk::Result::eSuccess);
    }
#endif
}

void Demo::init_vk_swapchain()
{
    create_surface();
    // Iterate over each queue to learn whether it supports presenting:
    std::vector<vk::Bool32> supportsPresent;
    for (uint32_t i = 0; i < static_cast<uint32_t>(queue_props.size()); i++)
    {
        auto supports = gpu.getSurfaceSupportKHR(i, surface);
        VERIFY(supports.result == vk::Result::eSuccess);
        supportsPresent.push_back(supports.value);
    }

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < static_cast<uint32_t>(queue_props.size()); i++)
    {
        if (queue_props[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            if (graphicsQueueFamilyIndex == UINT32_MAX)
            {
                graphicsQueueFamilyIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE)
            {
                graphicsQueueFamilyIndex = i;
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    if (presentQueueFamilyIndex == UINT32_MAX)
    {
        // If didn't find a queue that supports both graphics and present,
        // then
        // find a separate present queue.
        for (uint32_t i = 0; i < queue_props.size(); ++i)
        {
            if (supportsPresent[i] == VK_TRUE)
            {
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX)
    {
        ERR_EXIT("Could not find both graphics and present queues\n", "Swapchain Initialization Failure");
    }

    graphics_queue_family_index = graphicsQueueFamilyIndex;
    present_queue_family_index = presentQueueFamilyIndex;
    separate_present_queue = (graphics_queue_family_index != present_queue_family_index);

    create_device();

    graphics_queue = device.getQueue(graphics_queue_family_index, 0);
    if (!separate_present_queue)
    {
        present_queue = graphics_queue;
    }
    else
    {
        present_queue = device.getQueue(present_queue_family_index, 0);
    }

    // Get the list of VkFormat's that are supported:
    auto surface_formats_return = gpu.getSurfaceFormatsKHR(surface);
    VERIFY(surface_formats_return.result == vk::Result::eSuccess);

    vk::SurfaceFormatKHR surfaceFormat = pick_surface_format(surface_formats_return.value);
    format = surfaceFormat.format;
    color_space = surfaceFormat.colorSpace;

    quit = false;
    curFrame = 0;

    // Create semaphores to synchronize acquiring presentable buffers before
    // rendering and waiting for drawing to be complete before presenting
    auto const semaphoreCreateInfo = vk::SemaphoreCreateInfo();

    // Create fences that we can use to throttle if we get too far
    // ahead of the image presents
    auto const fence_ci = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
    for (uint32_t i = 0; i < FRAME_LAG; i++)
    {
        vk::Result result = device.createFence(&fence_ci, nullptr, &fences[i]);
        VERIFY(result == vk::Result::eSuccess);

        result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &image_acquired_semaphores[i]);
        VERIFY(result == vk::Result::eSuccess);

        result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &draw_complete_semaphores[i]);
        VERIFY(result == vk::Result::eSuccess);

        if (separate_present_queue)
        {
            result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &image_ownership_semaphores[i]);
            VERIFY(result == vk::Result::eSuccess);
        }
    }
    frame_index = 0;

    // Get Memory information and properties
    memory_properties = gpu.getMemoryProperties();
}

void Demo::prepare()
{
    prepare_init_cmd();

    prepare_buffers();
    prepare_depth();
    prepare_textures();
    prepare_cube_data_buffers();

    prepare_descriptor_layout();
    prepare_render_pass();
    prepare_pipeline();

    for (auto &swapchain_image_resource : swapchain_image_resources)
    {
        auto alloc_return = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
                                                              .setCommandPool(cmd_pool)
                                                              .setLevel(vk::CommandBufferLevel::ePrimary)
                                                              .setCommandBufferCount(1));
        VERIFY(alloc_return.result == vk::Result::eSuccess);
        swapchain_image_resource.cmd = alloc_return.value[0];
    }

    if (separate_present_queue)
    {
        auto present_cmd_pool_return =
            device.createCommandPool(vk::CommandPoolCreateInfo().setQueueFamilyIndex(present_queue_family_index));
        VERIFY(present_cmd_pool_return.result == vk::Result::eSuccess);
        present_cmd_pool = present_cmd_pool_return.value;

        for (auto &swapchain_image_resource : swapchain_image_resources)
        {
            auto alloc_cmd_return = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo()
                                                                      .setCommandPool(present_cmd_pool)
                                                                      .setLevel(vk::CommandBufferLevel::ePrimary)
                                                                      .setCommandBufferCount(1));
            VERIFY(alloc_cmd_return.result == vk::Result::eSuccess);
            swapchain_image_resource.graphics_to_present_cmd = alloc_cmd_return.value[0];
            build_image_ownership_cmd(swapchain_image_resource);
        }
    }

    prepare_descriptor_pool();
    prepare_descriptor_set();

    prepare_framebuffers();

    for (const auto &swapchain_image_resource : swapchain_image_resources)
    {
        draw_build_cmd(swapchain_image_resource);
    }

    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
    flush_init_cmd();
    if (staging_texture.buffer)
    {
        destroy_texture(staging_texture);
    }

    current_buffer = 0;
    prepared = true;
}

void Demo::prepare_buffers()
{
    vk::SwapchainKHR oldSwapchain = swapchain;

    // Check the surface capabilities and formats
    auto surface_capabilities_return = gpu.getSurfaceCapabilitiesKHR(surface);
    VERIFY(surface_capabilities_return.result == vk::Result::eSuccess);
    auto surfCapabilities = surface_capabilities_return.value;

    auto present_modes_return = gpu.getSurfacePresentModesKHR(surface);
    VERIFY(present_modes_return.result == vk::Result::eSuccess);
    auto present_modes = present_modes_return.value;

    vk::Extent2D swapchainExtent;
    // width and height are either both -1, or both not -1.
    if (surfCapabilities.currentExtent.width == static_cast<uint32_t>(-1))
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = width;
        swapchainExtent.height = height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
        width = surfCapabilities.currentExtent.width;
        height = surfCapabilities.currentExtent.height;
    }

    // The FIFO present mode is guaranteed by the spec to be supported
    // and to have no tearing.  It's a great default present mode to use.
    vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

    //  There are times when you may wish to use another present mode.  The
    //  following code shows how to select them, and the comments provide some
    //  reasons you may wish to use them.
    //
    // It should be noted that Vulkan 1.0 doesn't provide a method for
    // synchronizing rendering with the presentation engine's display.  There
    // is a method provided for throttling rendering with the display, but
    // there are some presentation engines for which this method will not work.
    // If an application doesn't throttle its rendering, and if it renders much
    // faster than the refresh rate of the display, this can waste power on
    // mobile devices.  That is because power is being spent rendering images
    // that may never be seen.

    // VK_PRESENT_MODE_IMMEDIATE_KHR is for applications that don't care
    // about
    // tearing, or have some way of synchronizing their rendering with the
    // display.
    // VK_PRESENT_MODE_MAILBOX_KHR may be useful for applications that
    // generally render a new presentable image every refresh cycle, but are
    // occasionally early.  In this case, the application wants the new
    // image
    // to be displayed instead of the previously-queued-for-presentation
    // image
    // that has not yet been displayed.
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR is for applications that generally
    // render a new presentable image every refresh cycle, but are
    // occasionally
    // late.  In this case (perhaps because of stuttering/latency concerns),
    // the application wants the late image to be immediately displayed,
    // even
    // though that may mean some tearing.

    if (presentMode != swapchainPresentMode)
    {
        for (const auto &mode : present_modes)
        {
            if (mode == presentMode)
            {
                swapchainPresentMode = mode;
                break;
            }
        }
    }

    if (swapchainPresentMode != presentMode)
    {
        ERR_EXIT("Present mode specified is not supported\n", "Present mode unsupported");
    }

    // Determine the number of VkImages to use in the swap chain.
    // Application desires to acquire 3 images at a time for triple
    // buffering
    uint32_t desiredNumOfSwapchainImages = 3;
    if (desiredNumOfSwapchainImages < surfCapabilities.minImageCount)
    {
        desiredNumOfSwapchainImages = surfCapabilities.minImageCount;
    }

    // If maxImageCount is 0, we can ask for as many images as we want,
    // otherwise
    // we're limited to maxImageCount
    if ((surfCapabilities.maxImageCount > 0) && (desiredNumOfSwapchainImages > surfCapabilities.maxImageCount))
    {
        // Application must settle for fewer images than desired:
        desiredNumOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    vk::SurfaceTransformFlagBitsKHR preTransform;
    if (surfCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
    {
        preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    }
    else
    {
        preTransform = surfCapabilities.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    std::array<vk::CompositeAlphaFlagBitsKHR, 4> compositeAlphaFlags = {
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
        vk::CompositeAlphaFlagBitsKHR::eInherit,
    };
    for (const auto &compositeAlphaFlag : compositeAlphaFlags)
    {
        if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlag)
        {
            compositeAlpha = compositeAlphaFlag;
            break;
        }
    }

    auto swapchain_return = device.createSwapchainKHR(vk::SwapchainCreateInfoKHR()
                                                          .setSurface(surface)
                                                          .setMinImageCount(desiredNumOfSwapchainImages)
                                                          .setImageFormat(format)
                                                          .setImageColorSpace(color_space)
                                                          .setImageExtent({swapchainExtent.width, swapchainExtent.height})
                                                          .setImageArrayLayers(1)
                                                          .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                                                          .setImageSharingMode(vk::SharingMode::eExclusive)
                                                          .setPreTransform(preTransform)
                                                          .setCompositeAlpha(compositeAlpha)
                                                          .setPresentMode(swapchainPresentMode)
                                                          .setClipped(true)
                                                          .setOldSwapchain(oldSwapchain));
    VERIFY(swapchain_return.result == vk::Result::eSuccess);
    swapchain = swapchain_return.value;

    // If we just re-created an existing swapchain, we should destroy the
    // old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (oldSwapchain)
    {
        device.destroySwapchainKHR(oldSwapchain);
    }

    auto swapchain_images_return = device.getSwapchainImagesKHR(swapchain);
    VERIFY(swapchain_images_return.result == vk::Result::eSuccess);
    swapchain_image_resources.resize(swapchain_images_return.value.size());

    for (uint32_t i = 0; i < swapchain_image_resources.size(); ++i)
    {
        auto color_image_view = vk::ImageViewCreateInfo()
                                    .setViewType(vk::ImageViewType::e2D)
                                    .setFormat(format)
                                    .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        swapchain_image_resources[i].image = swapchain_images_return.value[i];

        color_image_view.image = swapchain_image_resources[i].image;

        auto image_view_return = device.createImageView(color_image_view);
        VERIFY(image_view_return.result == vk::Result::eSuccess);
        swapchain_image_resources[i].view = image_view_return.value;
    }
}

void Demo::prepare_cube_data_buffers()
{
    mat4x4 VP;
    mat4x4_mul(VP, projection_matrix, view_matrix);

    mat4x4 MVP;
    mat4x4_mul(MVP, VP, model_matrix);

    vktexcube_vs_uniform data;
    memcpy(data.mvp, MVP, sizeof(MVP));
    //    dumpMatrix("MVP", MVP)

    for (int32_t i = 0; i < 12 * 3; i++)
    {
        data.position[i][0] = g_vertex_buffer_data[i * 3];
        data.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
        data.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
        data.position[i][3] = 1.0f;
        data.attr[i][0] = g_uv_buffer_data[2 * i];
        data.attr[i][1] = g_uv_buffer_data[2 * i + 1];
        data.attr[i][2] = 0;
        data.attr[i][3] = 0;
    }

    auto const buf_info = vk::BufferCreateInfo().setSize(sizeof(data)).setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

    for (auto &swapchain_image_resource : swapchain_image_resources)
    {
        auto result = device.createBuffer(&buf_info, nullptr, &swapchain_image_resource.uniform_buffer);
        VERIFY(result == vk::Result::eSuccess);

        vk::MemoryRequirements mem_reqs;
        device.getBufferMemoryRequirements(swapchain_image_resource.uniform_buffer, &mem_reqs);

        auto mem_alloc = vk::MemoryAllocateInfo().setAllocationSize(mem_reqs.size).setMemoryTypeIndex(0);

        bool const pass = memory_type_from_properties(
            mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            mem_alloc.memoryTypeIndex);
        VERIFY(pass);

        result = device.allocateMemory(&mem_alloc, nullptr, &swapchain_image_resource.uniform_memory);
        VERIFY(result == vk::Result::eSuccess);

        result = device.mapMemory(swapchain_image_resource.uniform_memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags(),
                                  &swapchain_image_resource.uniform_memory_ptr);
        VERIFY(result == vk::Result::eSuccess);

        memcpy(swapchain_image_resource.uniform_memory_ptr, &data, sizeof data);

        result = device.bindBufferMemory(swapchain_image_resource.uniform_buffer, swapchain_image_resource.uniform_memory, 0);
        VERIFY(result == vk::Result::eSuccess);
    }
}

void Demo::prepare_depth()
{
    depth.format = vk::Format::eD16Unorm;

    auto const image = vk::ImageCreateInfo()
                           .setImageType(vk::ImageType::e2D)
                           .setFormat(depth.format)
                           .setExtent({width, height, 1})
                           .setMipLevels(1)
                           .setArrayLayers(1)
                           .setSamples(vk::SampleCountFlagBits::e1)
                           .setTiling(vk::ImageTiling::eOptimal)
                           .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                           .setSharingMode(vk::SharingMode::eExclusive)
                           .setInitialLayout(vk::ImageLayout::eUndefined);

    auto result = device.createImage(&image, nullptr, &depth.image);
    VERIFY(result == vk::Result::eSuccess);

    vk::MemoryRequirements mem_reqs;
    device.getImageMemoryRequirements(depth.image, &mem_reqs);

    depth.mem_alloc.setAllocationSize(mem_reqs.size);
    depth.mem_alloc.setMemoryTypeIndex(0);

    auto const pass = memory_type_from_properties(mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                  depth.mem_alloc.memoryTypeIndex);
    VERIFY(pass);

    result = device.allocateMemory(&depth.mem_alloc, nullptr, &depth.mem);
    VERIFY(result == vk::Result::eSuccess);

    result = device.bindImageMemory(depth.image, depth.mem, 0);
    VERIFY(result == vk::Result::eSuccess);

    auto view = vk::ImageViewCreateInfo()
                    .setImage(depth.image)
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(depth.format)
                    .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
    if (force_errors)
    {
        // Intentionally force a bad pNext value to generate a validation layer error
        view.pNext = &image;
    }
    result = device.createImageView(&view, nullptr, &depth.view);
    VERIFY(result == vk::Result::eSuccess);
}

void Demo::prepare_descriptor_layout()
{
    std::array<vk::DescriptorSetLayoutBinding, 2> const layout_bindings = {
        vk::DescriptorSetLayoutBinding()
            .setBinding(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            .setPImmutableSamplers(nullptr),
        vk::DescriptorSetLayoutBinding()
            .setBinding(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(texture_count)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            .setPImmutableSamplers(nullptr)};

    auto const descriptor_layout = vk::DescriptorSetLayoutCreateInfo().setBindings(layout_bindings);

    auto result = device.createDescriptorSetLayout(&descriptor_layout, nullptr, &desc_layout);
    VERIFY(result == vk::Result::eSuccess);

    auto const pPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo().setSetLayouts(desc_layout);

    result = device.createPipelineLayout(&pPipelineLayoutCreateInfo, nullptr, &pipeline_layout);
    VERIFY(result == vk::Result::eSuccess);
}

void Demo::prepare_descriptor_pool()
{
    std::array<vk::DescriptorPoolSize, 2> const poolSizes = {
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(static_cast<uint32_t>(swapchain_image_resources.size())),
        vk::DescriptorPoolSize()
            .setType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(static_cast<uint32_t>(swapchain_image_resources.size()) * texture_count)};

    auto const descriptor_pool =
        vk::DescriptorPoolCreateInfo().setMaxSets(static_cast<uint32_t>(swapchain_image_resources.size())).setPoolSizes(poolSizes);

    auto result = device.createDescriptorPool(&descriptor_pool, nullptr, &desc_pool);
    VERIFY(result == vk::Result::eSuccess);
}

void Demo::prepare_descriptor_set()
{
    auto const alloc_info = vk::DescriptorSetAllocateInfo().setDescriptorPool(desc_pool).setSetLayouts(desc_layout);

    auto buffer_info = vk::DescriptorBufferInfo().setOffset(0).setRange(sizeof(vktexcube_vs_uniform));

    std::array<vk::DescriptorImageInfo, texture_count> tex_descs;
    for (uint32_t i = 0; i < texture_count; i++)
    {
        tex_descs[i].setSampler(textures[i].sampler);
        tex_descs[i].setImageView(textures[i].view);
        tex_descs[i].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    std::array<vk::WriteDescriptorSet, 2> writes;
    writes[0].setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eUniformBuffer).setPBufferInfo(&buffer_info);
    writes[1]
        .setDstBinding(1)
        .setDescriptorCount(texture_count)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setImageInfo(tex_descs);

    for (auto &swapchain_image_resource : swapchain_image_resources)
    {
        auto result = device.allocateDescriptorSets(&alloc_info, &swapchain_image_resource.descriptor_set);
        VERIFY(result == vk::Result::eSuccess);

        buffer_info.setBuffer(swapchain_image_resource.uniform_buffer);
        writes[0].setDstSet(swapchain_image_resource.descriptor_set);
        writes[1].setDstSet(swapchain_image_resource.descriptor_set);
        device.updateDescriptorSets(writes, {});
    }
}

void Demo::prepare_framebuffers()
{
    std::array<vk::ImageView, 2> attachments;
    attachments[1] = depth.view;

    for (auto &swapchain_image_resource : swapchain_image_resources)
    {
        attachments[0] = swapchain_image_resource.view;
        auto const framebuffer_return = device.createFramebuffer(vk::FramebufferCreateInfo()
                                                                     .setRenderPass(render_pass)
                                                                     .setAttachments(attachments)
                                                                     .setWidth(width)
                                                                     .setHeight(height)
                                                                     .setLayers(1));
        VERIFY(framebuffer_return.result == vk::Result::eSuccess);
        swapchain_image_resource.framebuffer = framebuffer_return.value;
    }
}

vk::ShaderModule Demo::prepare_fs()
{
    const uint32_t fragShaderCode[] = {
#include "cube.frag.inc"
    };

    frag_shader_module = prepare_shader_module(fragShaderCode, sizeof(fragShaderCode));

    return frag_shader_module;
}

void Demo::prepare_pipeline()
{
    vk::PipelineCacheCreateInfo const pipelineCacheInfo;
    auto result = device.createPipelineCache(&pipelineCacheInfo, nullptr, &pipelineCache);
    VERIFY(result == vk::Result::eSuccess);

    std::array<vk::PipelineShaderStageCreateInfo, 2> const shaderStageInfo = {
        vk::PipelineShaderStageCreateInfo().setStage(vk::ShaderStageFlagBits::eVertex).setModule(prepare_vs()).setPName("main"),
        vk::PipelineShaderStageCreateInfo().setStage(vk::ShaderStageFlagBits::eFragment).setModule(prepare_fs()).setPName("main")};

    vk::PipelineVertexInputStateCreateInfo const vertexInputInfo;

    auto const inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo().setTopology(vk::PrimitiveTopology::eTriangleList);

    // TODO: Where are pViewports and pScissors set?
    auto const viewportInfo = vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1);

    auto const rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
                                       .setDepthClampEnable(VK_FALSE)
                                       .setRasterizerDiscardEnable(VK_FALSE)
                                       .setPolygonMode(vk::PolygonMode::eFill)
                                       .setCullMode(vk::CullModeFlagBits::eBack)
                                       .setFrontFace(vk::FrontFace::eCounterClockwise)
                                       .setDepthBiasEnable(VK_FALSE)
                                       .setLineWidth(1.0f);

    auto const multisampleInfo = vk::PipelineMultisampleStateCreateInfo();

    auto const stencilOp =
        vk::StencilOpState().setFailOp(vk::StencilOp::eKeep).setPassOp(vk::StencilOp::eKeep).setCompareOp(vk::CompareOp::eAlways);

    auto const depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
                                      .setDepthTestEnable(VK_TRUE)
                                      .setDepthWriteEnable(VK_TRUE)
                                      .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                                      .setDepthBoundsTestEnable(VK_FALSE)
                                      .setStencilTestEnable(VK_FALSE)
                                      .setFront(stencilOp)
                                      .setBack(stencilOp);

    std::array<vk::PipelineColorBlendAttachmentState, 1> const colorBlendAttachments = {
        vk::PipelineColorBlendAttachmentState().setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                                  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)};

    auto const colorBlendInfo = vk::PipelineColorBlendStateCreateInfo().setAttachments(colorBlendAttachments);

    std::array<vk::DynamicState, 2> const dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    auto const dynamicStateInfo = vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamicStates);

    auto pipline_return = device.createGraphicsPipelines(pipelineCache, vk::GraphicsPipelineCreateInfo()
                                                                            .setStages(shaderStageInfo)
                                                                            .setPVertexInputState(&vertexInputInfo)
                                                                            .setPInputAssemblyState(&inputAssemblyInfo)
                                                                            .setPViewportState(&viewportInfo)
                                                                            .setPRasterizationState(&rasterizationInfo)
                                                                            .setPMultisampleState(&multisampleInfo)
                                                                            .setPDepthStencilState(&depthStencilInfo)
                                                                            .setPColorBlendState(&colorBlendInfo)
                                                                            .setPDynamicState(&dynamicStateInfo)
                                                                            .setLayout(pipeline_layout)
                                                                            .setRenderPass(render_pass));
    VERIFY(result == vk::Result::eSuccess);
    pipeline = pipline_return.value.at(0);

    device.destroyShaderModule(frag_shader_module);
    device.destroyShaderModule(vert_shader_module);
}

void Demo::prepare_render_pass()
{
    // The initial layout for the color and depth attachments will be LAYOUT_UNDEFINED
    // because at the start of the renderpass, we don't care about their contents.
    // At the start of the subpass, the color attachment's layout will be transitioned
    // to LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the depth stencil attachment's layout
    // will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the end of
    // the renderpass, the color attachment's layout will be transitioned to
    // LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as part of
    // the renderpass, no barriers are necessary.
    std::array<vk::AttachmentDescription, 2> const attachments = {
        vk::AttachmentDescription()
            .setFormat(format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
        vk::AttachmentDescription()
            .setFormat(depth.format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)};

    auto const color_reference = vk::AttachmentReference().setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    auto const depth_reference =
        vk::AttachmentReference().setAttachment(1).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    auto const subpass = vk::SubpassDescription()
                             .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                             .setColorAttachments(color_reference)
                             .setPDepthStencilAttachment(&depth_reference);

    vk::PipelineStageFlags stages = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
    std::array<vk::SubpassDependency, 2> const dependencies = {
        vk::SubpassDependency() // Depth buffer is shared between swapchain images
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(stages)
            .setDstStageMask(stages)
            .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            .setDependencyFlags(vk::DependencyFlags()),
        vk::SubpassDependency() // Image layout transition
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask(vk::AccessFlagBits())
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead)
            .setDependencyFlags(vk::DependencyFlags()),
    };

    const auto render_pass_result = device.createRenderPass(
        vk::RenderPassCreateInfo().setAttachments(attachments).setSubpasses(subpass).setDependencies(dependencies));
    VERIFY(render_pass_result.result == vk::Result::eSuccess);
    render_pass = render_pass_result.value;
}

vk::ShaderModule Demo::prepare_shader_module(const uint32_t *code, size_t size)
{
    const auto shader_module_return = device.createShaderModule(vk::ShaderModuleCreateInfo().setCodeSize(size).setPCode(code));
    VERIFY(shader_module_return.result == vk::Result::eSuccess);

    return shader_module_return.value;
}

void Demo::prepare_texture_buffer(const char *filename, texture_object &tex_obj)
{
    vk::SubresourceLayout tex_layout;

    if (!loadTexture(filename, nullptr, tex_layout, tex_obj.tex_width, tex_obj.tex_height))
    {
        ERR_EXIT("Failed to load textures", "Load Texture Failure");
    }

    auto const buffer_create_info = vk::BufferCreateInfo()
                                        .setSize(tex_obj.tex_width * tex_obj.tex_height * 4)
                                        .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
                                        .setSharingMode(vk::SharingMode::eExclusive);

    auto result = device.createBuffer(&buffer_create_info, nullptr, &tex_obj.buffer);
    VERIFY(result == vk::Result::eSuccess);

    vk::MemoryRequirements mem_reqs;
    device.getBufferMemoryRequirements(tex_obj.buffer, &mem_reqs);

    tex_obj.mem_alloc.setAllocationSize(mem_reqs.size);
    tex_obj.mem_alloc.setMemoryTypeIndex(0);

    vk::MemoryPropertyFlags requirements = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    auto pass = memory_type_from_properties(mem_reqs.memoryTypeBits, requirements, tex_obj.mem_alloc.memoryTypeIndex);
    VERIFY(pass == true);

    result = device.allocateMemory(&tex_obj.mem_alloc, nullptr, &(tex_obj.mem));
    VERIFY(result == vk::Result::eSuccess);

    result = device.bindBufferMemory(tex_obj.buffer, tex_obj.mem, 0);
    VERIFY(result == vk::Result::eSuccess);

    vk::SubresourceLayout layout;
    layout.rowPitch = tex_obj.tex_width * 4;
    auto data = device.mapMemory(tex_obj.mem, 0, tex_obj.mem_alloc.allocationSize);
    VERIFY(data.result == vk::Result::eSuccess);

    if (!loadTexture(filename, (uint8_t *)data.value, layout, tex_obj.tex_width, tex_obj.tex_height))
    {
        fprintf(stderr, "Error loading texture: %s\n", filename);
    }

    device.unmapMemory(tex_obj.mem);
}

void Demo::prepare_texture_image(const char *filename, texture_object &tex_obj, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                                 vk::MemoryPropertyFlags required_props)
{
    vk::SubresourceLayout tex_layout;
    if (!loadTexture(filename, nullptr, tex_layout, tex_obj.tex_width, tex_obj.tex_height))
    {
        ERR_EXIT("Failed to load textures", "Load Texture Failure");
    }

    auto const image_create_info = vk::ImageCreateInfo()
                                       .setImageType(vk::ImageType::e2D)
                                       .setFormat(vk::Format::eR8G8B8A8Unorm)
                                       .setExtent({tex_obj.tex_width, tex_obj.tex_height, 1})
                                       .setMipLevels(1)
                                       .setArrayLayers(1)
                                       .setSamples(vk::SampleCountFlagBits::e1)
                                       .setTiling(tiling)
                                       .setUsage(usage)
                                       .setSharingMode(vk::SharingMode::eExclusive)
                                       .setInitialLayout(vk::ImageLayout::ePreinitialized);

    auto result = device.createImage(&image_create_info, nullptr, &tex_obj.image);
    VERIFY(result == vk::Result::eSuccess);

    vk::MemoryRequirements mem_reqs;
    device.getImageMemoryRequirements(tex_obj.image, &mem_reqs);

    tex_obj.mem_alloc.setAllocationSize(mem_reqs.size);
    tex_obj.mem_alloc.setMemoryTypeIndex(0);

    auto pass = memory_type_from_properties(mem_reqs.memoryTypeBits, required_props, tex_obj.mem_alloc.memoryTypeIndex);
    VERIFY(pass == true);

    result = device.allocateMemory(&tex_obj.mem_alloc, nullptr, &tex_obj.mem);
    VERIFY(result == vk::Result::eSuccess);

    result = device.bindImageMemory(tex_obj.image, tex_obj.mem, 0);
    VERIFY(result == vk::Result::eSuccess);

    if (required_props & vk::MemoryPropertyFlagBits::eHostVisible)
    {
        auto const subres = vk::ImageSubresource().setAspectMask(vk::ImageAspectFlagBits::eColor).setMipLevel(0).setArrayLayer(0);
        vk::SubresourceLayout layout;
        device.getImageSubresourceLayout(tex_obj.image, &subres, &layout);

        auto data = device.mapMemory(tex_obj.mem, 0, tex_obj.mem_alloc.allocationSize);
        VERIFY(data.result == vk::Result::eSuccess);

        if (!loadTexture(filename, (uint8_t *)data.value, layout, tex_obj.tex_width, tex_obj.tex_height))
        {
            fprintf(stderr, "Error loading texture: %s\n", filename);
        }

        device.unmapMemory(tex_obj.mem);
    }

    tex_obj.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

void Demo::prepare_textures()
{
    vk::Format const tex_format = vk::Format::eR8G8B8A8Unorm;
    vk::FormatProperties props;
    gpu.getFormatProperties(tex_format, &props);

    for (uint32_t i = 0; i < texture_count; i++)
    {
        if ((props.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage) && !use_staging_buffer)
        {
            /* Device can texture using linear textures */
            prepare_texture_image(tex_files[i], textures[i], vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eSampled,
                                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            // Nothing in the pipeline needs to be complete to start, and don't allow fragment
            // shader to run until layout transition completes
            set_image_layout(textures[i].image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized,
                             textures[i].imageLayout, vk::AccessFlagBits(), vk::PipelineStageFlagBits::eTopOfPipe,
                             vk::PipelineStageFlagBits::eFragmentShader);
            staging_texture.image = vk::Image();
        }
        else if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage)
        {
            /* Must use staging buffer to copy linear texture to optimized */

            prepare_texture_buffer(tex_files[i], staging_texture);

            prepare_texture_image(tex_files[i], textures[i], vk::ImageTiling::eOptimal,
                                  vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                                  vk::MemoryPropertyFlagBits::eDeviceLocal);

            set_image_layout(textures[i].image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized,
                             vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits(), vk::PipelineStageFlagBits::eTopOfPipe,
                             vk::PipelineStageFlagBits::eTransfer);

            auto const subresource = vk::ImageSubresourceLayers()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setMipLevel(0)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1);

            auto const copy_region = vk::BufferImageCopy()
                                         .setBufferOffset(0)
                                         .setBufferRowLength(staging_texture.tex_width)
                                         .setBufferImageHeight(staging_texture.tex_height)
                                         .setImageSubresource(subresource)
                                         .setImageOffset({0, 0, 0})
                                         .setImageExtent({staging_texture.tex_width, staging_texture.tex_height, 1});

            cmd.copyBufferToImage(staging_texture.buffer, textures[i].image, vk::ImageLayout::eTransferDstOptimal, 1, &copy_region);

            set_image_layout(textures[i].image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eTransferDstOptimal,
                             textures[i].imageLayout, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer,
                             vk::PipelineStageFlagBits::eFragmentShader);
        }
        else
        {
            assert(!"No support for R8G8B8A8_UNORM as texture image format");
        }

        auto const samplerInfo = vk::SamplerCreateInfo()
                                     .setMagFilter(vk::Filter::eNearest)
                                     .setMinFilter(vk::Filter::eNearest)
                                     .setMipmapMode(vk::SamplerMipmapMode::eNearest)
                                     .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                                     .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                                     .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                                     .setMipLodBias(0.0f)
                                     .setAnisotropyEnable(VK_FALSE)
                                     .setMaxAnisotropy(1)
                                     .setCompareEnable(VK_FALSE)
                                     .setCompareOp(vk::CompareOp::eNever)
                                     .setMinLod(0.0f)
                                     .setMaxLod(0.0f)
                                     .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
                                     .setUnnormalizedCoordinates(VK_FALSE);

        auto result = device.createSampler(&samplerInfo, nullptr, &textures[i].sampler);
        VERIFY(result == vk::Result::eSuccess);

        auto const viewInfo = vk::ImageViewCreateInfo()
                                  .setImage(textures[i].image)
                                  .setViewType(vk::ImageViewType::e2D)
                                  .setFormat(tex_format)
                                  .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        result = device.createImageView(&viewInfo, nullptr, &textures[i].view);
        VERIFY(result == vk::Result::eSuccess);
    }
}

vk::ShaderModule Demo::prepare_vs()
{
    const uint32_t vertShaderCode[] = {
#include "cube.vert.inc"
    };

    vert_shader_module = prepare_shader_module(vertShaderCode, sizeof(vertShaderCode));

    return vert_shader_module;
}

void Demo::destroy_swapchain_related_resources()
{
    device.destroyDescriptorPool(desc_pool);

    device.destroyPipeline(pipeline);
    device.destroyPipelineCache(pipelineCache);
    device.destroyRenderPass(render_pass);
    device.destroyPipelineLayout(pipeline_layout);
    device.destroyDescriptorSetLayout(desc_layout);

    for (const auto &tex : textures)
    {
        device.destroyImageView(tex.view);
        device.destroyImage(tex.image);
        device.freeMemory(tex.mem);
        device.destroySampler(tex.sampler);
    }

    device.destroyImageView(depth.view);
    device.destroyImage(depth.image);
    device.freeMemory(depth.mem);

    for (const auto &resource : swapchain_image_resources)
    {
        device.destroyFramebuffer(resource.framebuffer);
        device.destroyImageView(resource.view);
        device.freeCommandBuffers(cmd_pool, {resource.cmd});
        device.destroyBuffer(resource.uniform_buffer);
        device.unmapMemory(resource.uniform_memory);
        device.freeMemory(resource.uniform_memory);
    }

    device.destroyCommandPool(cmd_pool);
    if (separate_present_queue)
    {
        device.destroyCommandPool(present_cmd_pool);
    }
}

void Demo::resize()
{
    // Don't react to resize until after first initialization.
    if (!prepared)
    {
        return;
    }

    // In order to properly resize the window, we must re-create the
    // swapchain
    // AND redo the command buffers, etc.
    //
    // First, perform part of the cleanup() function:
    prepared = false;
    auto result = device.waitIdle();
    VERIFY(result == vk::Result::eSuccess);
    destroy_swapchain_related_resources();

    // Second, re-perform the prepare() function, which will re-create the
    // swapchain.
    prepare();
}

void Demo::set_image_layout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                            vk::AccessFlags srcAccessMask, vk::PipelineStageFlags src_stages, vk::PipelineStageFlags dest_stages)
{
    assert(cmd);

    auto DstAccessMask = [](vk::ImageLayout const &layout)
    {
        vk::AccessFlags flags;

        switch (layout)
        {
        case vk::ImageLayout::eTransferDstOptimal:
            // Make sure anything that was copying from this image has
            // completed
            flags = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            flags = vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            flags = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            // Make sure any Copy or CPU writes to image are flushed
            flags = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            flags = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::ePresentSrcKHR:
            flags = vk::AccessFlagBits::eMemoryRead;
            break;
        default:
            break;
        }

        return flags;
    };

    cmd.pipelineBarrier(src_stages, dest_stages, vk::DependencyFlagBits(), {}, {},
                        vk::ImageMemoryBarrier()
                            .setSrcAccessMask(srcAccessMask)
                            .setDstAccessMask(DstAccessMask(newLayout))
                            .setOldLayout(oldLayout)
                            .setNewLayout(newLayout)
                            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                            .setImage(image)
                            .setSubresourceRange(vk::ImageSubresourceRange(aspectMask, 0, 1, 0, 1)));
}

void Demo::update_data_buffer()
{
    mat4x4 VP;
    mat4x4_mul(VP, projection_matrix, view_matrix);

    // Rotate around the Y axis
    mat4x4 Model;
    mat4x4_dup(Model, model_matrix);
    mat4x4_rotate_Y(model_matrix, Model, static_cast<float>(degreesToRadians(spin_angle)));
    mat4x4_orthonormalize(model_matrix, model_matrix);

    mat4x4 MVP;
    mat4x4_mul(MVP, VP, model_matrix);

    memcpy(swapchain_image_resources[current_buffer].uniform_memory_ptr, (const void *)&MVP[0][0], sizeof(MVP));
}

/* Convert ppm image data from header file into RGBA texture image */
#include "lunarg.ppm.h"
bool Demo::loadTexture(const char *filename, uint8_t *rgba_data, vk::SubresourceLayout &layout, uint32_t &width, uint32_t &height)
{
    (void)filename;
    char *cPtr;
    cPtr = (char *)lunarg_ppm;
    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "P6\n", 3))
    {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    sscanf(cPtr, "%u %u", &width, &height);
    if (rgba_data == nullptr)
    {
        return true;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    if ((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) || strncmp(cPtr, "255\n", 4))
    {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    for (uint32_t y = 0; y < height; y++)
    {
        uint8_t *rowPtr = rgba_data;
        for (uint32_t x = 0; x < width; x++)
        {
            memcpy(rowPtr, cPtr, 3);
            rowPtr[3] = 255; /* Alpha of 1 */
            rowPtr += 4;
            cPtr += 3;
        }
        rgba_data += layout.rowPitch;
    }
    return true;
}

bool Demo::memory_type_from_properties(uint32_t typeBits, vk::MemoryPropertyFlags requirements_mask, uint32_t &typeIndex)
{
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
    {
        if ((typeBits & 1) == 1)
        {
            // Type is available, does it match user properties?
            if ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
            {
                typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }

    // No memory types matched, return failure
    return false;
}

vk::SurfaceFormatKHR Demo::pick_surface_format(const std::vector<vk::SurfaceFormatKHR> &surface_formats)
{
    // Prefer non-SRGB formats...
    for (const auto &surface_format : surface_formats)
    {
        const vk::Format format = surface_format.format;

        if (format == vk::Format::eR8G8B8A8Unorm || format == vk::Format::eB8G8R8A8Unorm ||
            format == vk::Format::eA2B10G10R10UnormPack32 || format == vk::Format::eA2R10G10B10UnormPack32 ||
            format == vk::Format::eR16G16B16A16Sfloat)
        {
            return surface_format;
        }
    }

    printf("Can't find our preferred formats... Falling back to first exposed format. Rendering may be incorrect.\n");

    assert(surface_formats.size() >= 1);
    return surface_formats[0];
}

#if defined(VK_USE_PLATFORM_WIN32_KHR)
void Demo::run()
{
    if (!prepared)
    {
        return;
    }

    draw();
    curFrame++;

    if (frameCount != UINT32_MAX && curFrame == frameCount)
    {
        PostQuitMessage(validation_error);
    }
}

void Demo::create_window()
{
    WNDCLASSEX win_class;

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = connection; // hInstance
    win_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = nullptr;
    win_class.lpszClassName = name.c_str();
    win_class.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);

    // Register window class:
    if (!RegisterClassEx(&win_class))
    {
        // It didn't work, so try to give a useful error:
        printf("Unexpected error trying to start the application!\n");
        fflush(stdout);
        exit(1);
    }

    // Create window with the registered class:
    RECT wr = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    window = CreateWindowEx(0,
                            name.c_str(),         // class name
                            name.c_str(),         // app name
                            WS_OVERLAPPEDWINDOW | // window style
                                WS_VISIBLE | WS_SYSMENU,
                            100, 100,           // x/y coords
                            wr.right - wr.left, // width
                            wr.bottom - wr.top, // height
                            nullptr,            // handle to parent
                            nullptr,            // handle to menu
                            connection,         // hInstance
                            nullptr);           // no extra parameters

    if (!window)
    {
        // It didn't work, so try to give a useful error:
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }

    // Window client area size must be at least 1 pixel high, to prevent
    // crash.
    minsize.x = GetSystemMetrics(SM_CXMINTRACK);
    minsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;
}
#endif

#if _WIN32
// Include header required for parsing the command line options.
#include <shellapi.h>

Demo demo;

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        PostQuitMessage(validation_error);
        break;
    case WM_PAINT:
        if (!demo.in_callback)
        {
            demo.run();
        }
        break;
    case WM_GETMINMAXINFO: // set window's minimum size
        ((MINMAXINFO *)lParam)->ptMinTrackSize = demo.minsize;
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        // Resize the application to the new window size, except when
        // it was minimized. Vulkan doesn't support images or swapchains
        // with width=0 and height=0.
        if (wParam != SIZE_MINIMIZED)
        {
            demo.width = lParam & 0xffff;
            demo.height = (lParam & 0xffff0000) >> 16;
            demo.resize();
        }
        break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(validation_error);
            break;
        case VK_LEFT:
            demo.spin_angle -= demo.spin_increment;
            break;
        case VK_RIGHT:
            demo.spin_angle += demo.spin_increment;
            break;
        case VK_SPACE:
            demo.pause = !demo.pause;
            break;
        }
        return 0;
    default:
        break;
    }

    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    // TODO: Gah.. refactor. This isn't 1989.
    MSG msg;   // message
    bool done; // flag saying when app is complete
    int argc;
    char **argv;

    // Ensure wParam is initialized.
    msg.wParam = 0;

    // Use the CommandLine functions to get the command line arguments.
    // Unfortunately, Microsoft outputs
    // this information as wide characters for Unicode, and we simply want the
    // Ascii version to be compatible
    // with the non-Windows side.  So, we have to convert the information to
    // Ascii character strings.
    LPWSTR *commandLineArgs = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (nullptr == commandLineArgs)
    {
        argc = 0;
    }

    if (argc > 0)
    {
        argv = (char **)malloc(sizeof(char *) * argc);
        if (argv == nullptr)
        {
            argc = 0;
        }
        else
        {
            for (int iii = 0; iii < argc; iii++)
            {
                size_t wideCharLen = wcslen(commandLineArgs[iii]);
                size_t numConverted = 0;

                argv[iii] = (char *)malloc(sizeof(char) * (wideCharLen + 1));
                if (argv[iii] != nullptr)
                {
                    wcstombs_s(&numConverted, argv[iii], wideCharLen + 1, commandLineArgs[iii], wideCharLen + 1);
                }
            }
        }
    }
    else
    {
        argv = nullptr;
    }

    demo.init(argc, argv);

    // Free up the items we had to allocate for the command line arguments.
    if (argc > 0 && argv != nullptr)
    {
        for (int iii = 0; iii < argc; iii++)
        {
            if (argv[iii] != nullptr)
            {
                free(argv[iii]);
            }
        }
        free(argv);
    }

    demo.connection = hInstance;
    demo.name = "Vulkan Cube";
    demo.create_window();
    demo.init_vk_swapchain();

    demo.prepare();

    done = false; // initialize loop condition variable

    // main message loop
    while (!done)
    {
        if (demo.pause)
        {
            const BOOL succ = WaitMessage();

            if (!succ)
            {
                const auto &suppress_popups = demo.suppress_popups;
                ERR_EXIT("WaitMessage() failed on paused demo", "event loop error");
            }
        }

        PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        if (msg.message == WM_QUIT) // check for a quit message
        {
            done = true; // if found, quit app
        }
        else
        {
            /* Translate and dispatch to event queue*/
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        RedrawWindow(demo.window, nullptr, nullptr, RDW_INTERNALPAINT);
    }

    demo.cleanup();

    return static_cast<int>(msg.wParam);
}

#endif
