#pragma once
#include <cstdint>
#include <vector>
#include <unordered_set>

namespace slippygl::render 
{
	using TexHandle = unsigned int; // GLuint

	/**
	 * OpenGL texture manager
	 * RGBA8 image -> texture create/destroy
	 * CLAMP_TO_EDGE, NEAREST filtering (for sharp tile rendering)
	 */
	class TextureManager 
	{
	public:
		TextureManager() = default;
		~TextureManager();

		// Non-copyable
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		/**
		 * Create texture from RGBA8 pixel data
		 * @param w Image width
		 * @param h Image height
		 * @param pixels RGBA8 pixel data (w*h*4 bytes)
		 * @return OpenGL texture handle (0 on failure)
		 */
		TexHandle createRGBA8(int w, int h, const std::uint8_t* pixels);

		/**
		 * Destroy texture
		 */
		void destroy(TexHandle tex);

		/**
		 * Destroy all textures
		 */
		void destroyAll();

	private:
		std::unordered_set<TexHandle> textures_;
	};
}
