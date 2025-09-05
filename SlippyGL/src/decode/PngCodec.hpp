#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "Image.hpp"

namespace slippygl::decode
{
// PNG 바이트 → RGBA8 Image
// desiredChannels 기본 4 (OpenGL 텍스처에 적합)
class PngCodec
{
public:
	static bool decode(const std::vector<std::uint8_t>& pngBytes,
		Image& out,
		const int32_t desiredChannels = 4,
		std::string* err = nullptr);
};

} // namespace slippygl::decode
