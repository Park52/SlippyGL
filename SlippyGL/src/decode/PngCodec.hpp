#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "Image.hpp"

namespace slippygl::decode
{
/**
 * PNG 바이트 배열을 RGBA/RGB/Grayscale Image로 디코딩하는 유틸리티
 * STB 라이브러리를 내부적으로 사용함
 */
class PngCodec
{
public:
    /**
     * PNG 바이트를 Image로 디코딩
     * @param pngBytes PNG 포맷의 바이트 데이터
     * @param out 디코딩된 이미지가 저장될 Image 객체
     * @param desiredChannels 원하는 채널 수 (0=원본, 1=Gray, 2=GrayAlpha, 3=RGB, 4=RGBA)
     * @param err 에러 메시지가 저장될 문자열 포인터 (nullptr 가능)
     * @return 성공시 true, 실패시 false
     * @note 실패시 out은 초기화된 상태로 유지됨
     */
    static bool decode(const std::vector<std::uint8_t>& pngBytes,
                      Image& out,
                      const std::int32_t desiredChannels = 4,
                      std::string* err = nullptr) noexcept;
};

} // namespace slippygl::decode
