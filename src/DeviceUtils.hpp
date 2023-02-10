#ifndef __VL_DEVICEUTILS_HPP__
#define __VL_DEVICEUTILS_HPP__

#include "Vulkan.hpp"
#include <ntl/NTL.hpp>

namespace vl
{
    /// @brief 逻辑设备工具
    class DeviceUtils : public ntl::Object
    {
    public:
        using SelfType = DeviceUtils;
        using ParentType = ntl::Object;

    public:
        constexpr DeviceUtils() noexcept = default;
        constexpr explicit DeviceUtils(const SelfType &) noexcept = default;
        ~DeviceUtils() override = default;

    public:
        constexpr SelfType &operator=(const SelfType &from) = default;

        public:
        
    };
} // namespace vl

#endif