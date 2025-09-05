#pragma once
#include <cstdint>
#include <vector>

namespace slippygl::decode 
{
struct Image 
{
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t channels = 0;                 // 실제 결과 채널 수 (예: 4)
    std::vector<std::uint8_t> pixels;          // RGBA8 등

    // 유효성 검사 (크기와 데이터 일관성 확인)
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

    // 픽셀 데이터 크기
    std::size_t sizeBytes() const noexcept
    {
        return pixels.size();
    }

    // 예상 픽셀 데이터 크기
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

    // 이미지 초기화
    void clear() noexcept
    {
        width = 0;
        height = 0;
        channels = 0;
        pixels.clear();
    }

    // 빈 이미지인지 확인
    bool empty() const noexcept
    {
        return pixels.empty();
    }
};

} // namespace slippygl::decode
