#ifndef __VL_DEFAULTQUEUEFAMILYINDICES_HPP__
#define __VL_DEFAULTQUEUEFAMILYINDICES_HPP__

#include <optional>
#include "QueueFamilyIndices.hpp"

namespace vl
{
    /// @brief 队列系列索引
    class DefaultQueueFamilyIndices : public QueueFamilyIndices
    {
    public:
        using SelfType = DefaultQueueFamilyIndices;
        using ParentType = QueueFamilyIndices;

    public:
        /// @brief 图形系列
        std::optional<unsigned int> m_graphics_family;

        /// @brief 计算系列
        std::optional<unsigned int> m_compute_family;

        /// @brief 转移系列
        std::optional<unsigned int> m_transfer_family;

        /// @brief 稀少绑定系列
        std::optional<unsigned int> m_sparse_binding_family;

    public:
        DefaultQueueFamilyIndices() = default;
        explicit DefaultQueueFamilyIndices(const SelfType &from) = default;
        ~DefaultQueueFamilyIndices() override = default;

    public:
        SelfType &operator=(const SelfType &from) noexcept = default;

    public:
        virtual ntl::String format();

    public:
        bool find(const std::vector<VkQueueFamilyProperties> &queue_families) override;
        bool is_complete() override;
    };
} // namespace vl

#endif