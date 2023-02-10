#ifndef __VL_QUEUEFAMILYINDICES_HPP__
#define __VL_QUEUEFAMILYINDICES_HPP__

#include <vector>
#include "Vulkan.hpp"
#include <ntl/NTL.hpp>

namespace vl
{
    /// @brief 队列系列索引
    class QueueFamilyIndices : public ntl::Object
    {
    public:
        using SelfType = QueueFamilyIndices;
        using ParentType = ntl::Object;

    public:
        constexpr QueueFamilyIndices() noexcept = default;
        constexpr explicit QueueFamilyIndices(const SelfType &from) noexcept = default;
        ~QueueFamilyIndices() override = default;

    public:
        constexpr SelfType &operator=(const SelfType &from) noexcept = default;

    public:
        /// @brief 查找队列系列索引并赋值
        /// @param queue_families 队列系列属性
        /// @return 是否成功找到
        virtual bool find(const std::vector<VkQueueFamilyProperties> &queue_families) = 0;

        /// @brief 队列是否完整
        /// @return 是否完整
        virtual bool is_complete() = 0;
    };
} // namespace vl

#endif