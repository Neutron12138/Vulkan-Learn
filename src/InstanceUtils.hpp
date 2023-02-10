#ifndef __VL_INSTANCEUTILS_HPP__
#define __VL_INSTANCEUTILS_HPP__

#include "Vulkan.hpp"
#include <ntl/NTL.hpp>

namespace vl
{
    /// @brief 实例创建工具
    class InstanceUtils : public ntl::Object
    {
    public:
        using SelfType = InstanceUtils;
        using ParentType = ntl::Object;

    public:
        constexpr InstanceUtils() noexcept = default;
        constexpr explicit InstanceUtils(const SelfType &from) noexcept = default;
        ~InstanceUtils() override = default;

    public:
        constexpr SelfType &operator=(const SelfType &from) noexcept = default;

    public:
        /// @brief 创建一个vk实例
        /// @param name 实例名
        /// @return 结果
        static vk::ResultValue<vk::Instance> create_instance(const std::string &name, const std::vector<std::string> &validation_layers);

        /// @brief 检查层支持
        /// @param validation_layers 验证层
        /// @param layers 所有的层
        /// @return 不支持的曾列表
        static std::vector<std::string> check_layer_support(const std::vector<std::string> &validation_layers, const std::vector<vk::LayerProperties> &layers);
    };
} // namespace vl

#endif