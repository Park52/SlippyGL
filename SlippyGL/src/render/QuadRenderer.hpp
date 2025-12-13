#pragma once
#include "TextureManager.hpp"
#include <glm/glm.hpp>

namespace slippygl::render 
{
	/**
	 * Screen coordinate quad definition
	 * Top-left origin, Y-axis down
	 */
	struct Quad 
	{
		// Destination rectangle (dst): x, y, w, h in pixels (top-left origin)
		int x = 0, y = 0, w = 256, h = 256;
		// Texture source rectangle (src): in pixels (0..texW, 0..texH)
		int sx = 0, sy = 0, sw = 256, sh = 256;
	};

	/**
	 * Renders textured quads with orthographic projection
	 * Pixel-accurate placement, suitable for tile map rendering
	 */
	class QuadRenderer 
	{
	public:
		QuadRenderer() = default;
		~QuadRenderer();

		// Non-copyable
		QuadRenderer(const QuadRenderer&) = delete;
		QuadRenderer& operator=(const QuadRenderer&) = delete;

		/**
		 * Initialize VBO/VAO/Shader program
		 * @return true on success
		 */
		bool init();

		/**
		 * Release resources
		 */
		void shutdown();

		/**
		 * Draw textured quad (generates internal projection matrix)
		 * @param tex Texture handle
		 * @param q Quad info (in world coordinates)
		 * @param texFullW Texture full width
		 * @param texFullH Texture full height
		 * @param fbW Framebuffer width
		 * @param fbH Framebuffer height
		 */
		void draw(TexHandle tex, const Quad& q, int texFullW, int texFullH, int fbW, int fbH);

		/**
		 * Draw textured quad with external MVP matrix
		 * @param tex Texture handle
		 * @param q Quad info (in world coordinates)
		 * @param texFullW Texture full width
		 * @param texFullH Texture full height
		 * @param mvp Model-View-Projection matrix
		 */
		void draw(TexHandle tex, const Quad& q, int texFullW, int texFullH, const glm::mat4& mvp);

	private:
		unsigned int vao_ = 0;
		unsigned int vbo_ = 0;
		unsigned int program_ = 0;

		// Uniform locations
		int uProjLoc_ = -1;
		int uTexLoc_ = -1;

		bool compileShaders();
	};
}
