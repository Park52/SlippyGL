#pragma once
#include <cstdint>
#include <vector>

namespace slippygl::decode 
{
struct Image 
{
int32_t width = 0;
int32_t height = 0;
int32_t channels = 0;                 // 실제 결과 채널 수 (예: 4)
std::vector<std::uint8_t> pixels;       // RGBA8 등

const bool valid() const
{
	return (width > 0) && (height > 0) && (!pixels.empty());
}
const std::size_t sizeBytes() const
{
	return pixels.size();
}
};

} // namespace slippygl::decode
