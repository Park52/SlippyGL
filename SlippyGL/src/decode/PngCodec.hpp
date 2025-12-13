#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "Image.hpp"

namespace slippygl::decode
{
/**
 * Utility to decode PNG byte array to RGBA/RGB/Grayscale Image
 * Uses STB library internally
 */
class PngCodec
{
public:
    /**
     * Decode PNG bytes to Image
     * @param pngBytes PNG format byte data
     * @param out Image object where decoded image will be stored
     * @param desiredChannels Desired channel count (0=original, 1=Gray, 2=GrayAlpha, 3=RGB, 4=RGBA)
     * @param err Pointer to string for error message (can be nullptr)
     * @return true on success, false on failure
     * @note On failure, out remains in initialized state
     */
    static bool decode(const std::vector<std::uint8_t>& pngBytes,
                      Image& out,
                      const std::int32_t desiredChannels = 4,
                      std::string* err = nullptr) noexcept;
};

} // namespace slippygl::decode
