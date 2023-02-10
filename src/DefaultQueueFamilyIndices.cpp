#ifndef __VL_DEFAULTQUEUEFAMILYINDICES_CPP__
#define __VL_DEFAULTQUEUEFAMILYINDICES_CPP__

#include "DefaultQueueFamilyIndices.hpp"

namespace vl
{
    ntl::String
    DefaultQueueFamilyIndices::format()
    {
        auto output = [](const std::optional<unsigned int> &value) -> ntl::String
        {
            return value.has_value() ? ntl::StringUtils::to_string(*value) : NTL_STRING("not found");
        };

        ntl::StringStream sstr;
        sstr << std::endl
             << NTL_STRING("\tgraphics:") << output(m_graphics_family) << std::endl
             << NTL_STRING("\tcompute:") << output(m_compute_family) << std::endl
             << NTL_STRING("\ttransfer:") << output(m_transfer_family) << std::endl
             << NTL_STRING("\tsparse binding:") << output(m_sparse_binding_family) << std::endl;
        return sstr.str();
    }

    bool
    DefaultQueueFamilyIndices::find(
        const std::vector<VkQueueFamilyProperties> &queue_families)
    {
        for (size_t i = 0; i < queue_families.size(); i++)
        {
            const VkQueueFamilyProperties &family = queue_families.at(i);

            if ((family.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !m_graphics_family.has_value())
                m_graphics_family = i;

            if ((family.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !m_compute_family.has_value())
                m_compute_family = i;

            if ((family.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !m_transfer_family.has_value())
                m_transfer_family = i;

            if ((family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) &&
                !m_sparse_binding_family.has_value())
                m_sparse_binding_family = i;
        }

        if (is_complete())
            return true;
        else
            return false;
    }

    bool
    DefaultQueueFamilyIndices::is_complete()
    {
        return m_graphics_family.has_value() &&
               m_compute_family.has_value() &&
               m_transfer_family.has_value() &&
               m_sparse_binding_family.has_value();
    }
} // namespace vl

#endif