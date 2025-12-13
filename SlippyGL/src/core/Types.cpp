#include "Types.hpp"
#include <string>

using namespace slippygl::core;

std::string TileID::toString() const 
{
    return std::to_string(z_) + "/" + std::to_string(x_) + "/" + std::to_string(y_);
}
