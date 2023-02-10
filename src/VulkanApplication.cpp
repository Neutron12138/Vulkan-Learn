#ifndef __VL_VULKANAPPLICATION_CPP__
#define __VL_VULKANAPPLICATION_CPP__

#include <SFML/Graphics.hpp>
#include "VulkanApplication.hpp"

namespace vl
{
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
        if (!SetupDebugMessenger())
            quit(EXIT_FAILURE);
        if (!PickPhysicalDevice())
            quit(EXIT_FAILURE);
        if (!CreateDevice())
            quit(EXIT_FAILURE);
    }

} // namespace vl

#endif