#pragma once
#include <string>
#include "../core/Types.hpp"  // TileID

namespace slippygl::net 
{

class TileEndpoint 
{
public:
    explicit TileEndpoint(std::string base = "https://tile.openstreetmap.org")
        : baseUrl_(std::move(base)) {}

    const std::string& baseUrl() const noexcept { return baseUrl_; }
    TileEndpoint& setBaseUrl(std::string v) noexcept { baseUrl_ = std::move(v); return *this; }

    std::string rasterUrl(const slippygl::core::TileID& id) const noexcept; // z/x/y.png
    std::string mvtUrl(const slippygl::core::TileID& id) const noexcept;    // z/x/y.mvt or .pbf (미래)

private:
    std::string baseUrl_;
};

} // namespace slippygl::net
