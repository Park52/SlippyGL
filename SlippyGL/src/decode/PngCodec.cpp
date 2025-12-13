#include "PngCodec.hpp"

// STB Image library configuration (must be before include)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO          // Disable file I/O (memory only)
#define STBI_ONLY_PNG          // PNG only

// vcpkg stb package uses this path
#include <stb_image.h>

namespace slippygl::decode
{

// RAII wrapper for stb_image memory safety
class StbImageRAII 
{
public:
    explicit StbImageRAII(unsigned char* ptr) noexcept : ptr_(ptr) {}
    ~StbImageRAII() noexcept { if (ptr_) stbi_image_free(ptr_); }
    
    // Non-copyable, non-movable
    StbImageRAII(const StbImageRAII&) = delete;
    StbImageRAII& operator=(const StbImageRAII&) = delete;
    StbImageRAII(StbImageRAII&&) = delete;
    StbImageRAII& operator=(StbImageRAII&&) = delete;

    unsigned char* get() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    unsigned char* ptr_;
};

bool PngCodec::decode(const std::vector<std::uint8_t>& pngBytes,
                     Image& out,
                     const std::int32_t desiredChannels,
                     std::string* err) noexcept
{
    // Reset output
    out.clear();

    // Validate input
    if (pngBytes.empty())
    {
        if (err) 
        {
            *err = "Empty PNG data";
        }
        return false;
    }

    // Validate channel count
    if (desiredChannels != 0 && 
        (desiredChannels < 1 || desiredChannels > 4))
    {
        if (err) 
        {
            *err = "Invalid desiredChannels: must be 0 (auto) or 1-4";
        }
        return false;
    }

    // Size limit (memory protection, 256MB max)
    if (pngBytes.size() > 256 * 1024 * 1024) 
    {
        if (err) 
        {
            *err = "PNG data too large (>256MB)";
        }
        return false;
    }

    try {
        // STB decoding
        std::int32_t w = 0, h = 0, originalChannels = 0;
        const std::int32_t requestedChannels = (desiredChannels == 0) ? 0 : desiredChannels;
        
        unsigned char* data = stbi_load_from_memory(
            pngBytes.data(), 
            static_cast<int>(pngBytes.size()), 
            &w, &h, &originalChannels, 
            requestedChannels
        );

        // RAII memory management
        StbImageRAII dataGuard(data);
        
        if (!dataGuard) 
        {
            if (err) 
            {
                const char* stbErr = stbi_failure_reason();
                *err = stbErr ? std::string("STB decode failed: ") + stbErr 
                             : "STB decode failed: unknown error";
            }
            return false;
        }

        // Validate result
        if (w <= 0 || h <= 0 || originalChannels <= 0) 
        {
            if (err) 
            {
                *err = "Invalid image dimensions or channels";
            }
            return false;
        }

        // Determine final channel count
        const std::int32_t finalChannels = (desiredChannels == 0) ? originalChannels : desiredChannels;
        
        // Calculate pixel data size
        const std::size_t pixelDataSize = static_cast<std::size_t>(w) * 
                                         static_cast<std::size_t>(h) * 
                                         static_cast<std::size_t>(finalChannels);

        // Set result
        out.width = w;
        out.height = h;
        out.channels = finalChannels;
        out.pixels.assign(data, data + pixelDataSize);

        return true;
    }
    catch (...) {
        // Safety catch for any unexpected exceptions
        if (err) 
        {
            *err = "Unexpected error during PNG decoding";
        }
        out.clear();
        return false;
    }
}

} // namespace slippygl::decode
