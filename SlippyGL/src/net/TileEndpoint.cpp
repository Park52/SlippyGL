#include "TileEndpoint.hpp"

namespace slippygl::net 
{

using slippygl::core::TileID;

std::string TileEndpoint::rasterUrl(const TileID& id) const noexcept
{
    // e.g., https://tile.openstreetmap.org/z/x/y.png
    return baseUrl_ + "/" + std::to_string(id.z()) + "/" + std::to_string(id.x()) + "/" + std::to_string(id.y()) + ".png";
}

std::string TileEndpoint::mvtUrl(const TileID& id) const noexcept
{
    // 일반적으로 .pbf 확장자를 많이 사용 (서버마다 다름)
    return baseUrl_ + "/" + std::to_string(id.z()) + "/" + std::to_string(id.x()) + "/" + std::to_string(id.y()) + ".pbf";
}

} // namespace slippygl::net