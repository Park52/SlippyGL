#pragma once
#include <cstdint>
#include <vector>

namespace slippygl::decode 
{
struct Image 
{
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t channels = 0;                 // Actual result channel count (e.g., 4)
    std::vector<std::uint8_t> pixels;          // RGBA8 etc.

    // Validity check (verify size and data consistency)
    bool valid() const noexcept
    {
        if (width <= 0 || height <= 0 || channels <= 0) 
        {
            return false;
        }
        const std::size_t expectedSize = static_cast<std::size_t>(width) * 
                                        static_cast<std::size_t>(height) * 
                                        static_cast<std::size_t>(channels);
        return pixels.size() == expectedSize;
    }

    // Pixel data size in bytes
    std::size_t sizeBytes() const noexcept
    {
        return pixels.size();
    }

    // Expected pixel data size
    std::size_t expectedSizeBytes() const noexcept
    {
        if (width <= 0 || height <= 0 || channels <= 0) 
        {
            return 0;
        }
        return static_cast<std::size_t>(width) * 
               static_cast<std::size_t>(height) * 
               static_cast<std::size_t>(channels);
    }

    // Clear image
    void clear() noexcept
    {
        width = 0;
        height = 0;
        channels = 0;
        pixels.clear();
    }

    // Check if image is empty
    bool empty() const noexcept
    {
        return pixels.empty();
    }
};

} // namespace slippygl::decode
