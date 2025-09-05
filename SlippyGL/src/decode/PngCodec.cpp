#include "PngCodec.hpp"

// STB 라이브러리 구현체 포함 (헤더 인클루드 전에 정의해야 함!)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO          // 파일 I/O 비활성화(메모리 만 사용)
#define STBI_ONLY_PNG          // 선택: PNG만 디코드. (원하면 주석처리)

#include <stb_image.h>
// vcpkg의 stb: 보통 <stb_image.h> 경로 제공

namespace slippygl::decode
{
bool PngCodec::decode(const std::vector<std::uint8_t>& pngBytes,
	Image& out,
	const int32_t desiredChannels,
	std::string* err)
{
	out = Image{}; // reset

	if (pngBytes.empty())
	{
		if (err)
		{
			*err = "empty input";
		}
		return false;
	}

	if ((desiredChannels != 0) && (desiredChannels != 1 &&
		desiredChannels != 2 && desiredChannels != 3 && desiredChannels != 4))
	{
		if (err)
		{
			*err = "invalid desiredChannels";
		}
		return false;
	}

	int32_t w = 0, h = 0, ch = 0;
	// stbi_load_from_memory: 실패 시 nullptr, stbi_failure_reason()로 사유
	unsigned char* data = stbi_load_from_memory(pngBytes.data(), static_cast<int>(pngBytes.size()), &w, &h, &ch, desiredChannels == 0 ? 0 : desiredChannels);
	if (!data)
	{
		if (err)
		{
			*err = stbi_failure_reason() ? stbi_failure_reason() : "stb_image failed";
		}
		return false;
	}

	out.width = w;
	out.height = h;
	out.channels = (desiredChannels == 0) ? ch : desiredChannels;
	const std::size_t bytes = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * static_cast<std::size_t>(out.channels);
	out.pixels.assign(data, data + bytes);

	stbi_image_free(data);
	return true;
}

} // namespace slippygl::decode
