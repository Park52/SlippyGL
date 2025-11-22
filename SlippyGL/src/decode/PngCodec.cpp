#include "PngCodec.hpp"
#include <stb_image.h>
namespace slippygl::decode
{

// RAII 래퍼로 메모리 안전성 보장
class StbImageRAII 
{
public:
    explicit StbImageRAII(unsigned char* ptr) noexcept : ptr_(ptr) {}
    ~StbImageRAII() noexcept { if (ptr_) stbi_image_free(ptr_); }
    
    // 복사/이동 금지
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
    // 결과 초기화
    out.clear();

    // 입력 검증
    if (pngBytes.empty())
    {
        if (err) 
        {
            *err = "Empty PNG data";
        }
        return false;
    }

    // 채널 수 검증
    if (desiredChannels != 0 && 
        (desiredChannels < 1 || desiredChannels > 4))
    {
        if (err) 
        {
            *err = "Invalid desiredChannels: must be 0 (auto) or 1-4";
        }
        return false;
    }

    // 크기 제한 (메모리 보호)
    if (pngBytes.size() > 256 * 1024 * 1024) 
    { // 256MB 제한
        if (err) 
        {
            *err = "PNG data too large (>256MB)";
        }
        return false;
    }

    try {
        // STB 디코딩
        std::int32_t w = 0, h = 0, originalChannels = 0;
        const std::int32_t requestedChannels = (desiredChannels == 0) ? 0 : desiredChannels;
        
        unsigned char* data = stbi_load_from_memory(
            pngBytes.data(), 
            static_cast<int>(pngBytes.size()), 
            &w, &h, &originalChannels, 
            requestedChannels
        );

        // RAII로 메모리 관리
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

        // 결과 검증
        if (w <= 0 || h <= 0 || originalChannels <= 0) 
        {
            if (err) 
            {
                *err = "Invalid image dimensions or channels";
            }
            return false;
        }

        // 최종 채널 수 결정
        const std::int32_t finalChannels = (desiredChannels == 0) ? originalChannels : desiredChannels;
        
        // 픽셀 데이터 크기 계산
        const std::size_t pixelDataSize = static_cast<std::size_t>(w) * 
                                         static_cast<std::size_t>(h) * 
                                         static_cast<std::size_t>(finalChannels);

        // 결과 설정
        out.width = w;
        out.height = h;
        out.channels = finalChannels;
        out.pixels.assign(data, data + pixelDataSize);

        return true;
    }
    catch (...) {
        // STB 함수들이 예외를 던질 수 있으므로 안전 장치
        if (err) 
        {
            *err = "Unexpected error during PNG decoding";
        }
        out.clear();
        return false;
    }
}

} // namespace slippygl::decode
