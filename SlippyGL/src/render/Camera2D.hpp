#pragma once

#include <glm/glm.hpp>
#include <algorithm>

namespace slippygl::render
{
    /**
     * 2D Camera for tile map rendering
     * - Y-down coordinate system (consistent with screen coordinates)
     * - Supports pan (drag) and zoom (scroll wheel)
     * - Cursor-centered zoom for intuitive navigation
     */
    class Camera2D
    {
    public:
        Camera2D() = default;
        ~Camera2D() = default;

        // Non-copyable (simple value type, but explicit)
        Camera2D(const Camera2D&) = default;
        Camera2D& operator=(const Camera2D&) = default;

        /**
         * Pan camera by screen delta
         * @param dx Screen X delta (positive = drag right, camera moves left in world)
         * @param dy Screen Y delta (positive = drag down, camera moves up in world)
         */
        void pan(float dx, float dy) noexcept;

        /**
         * Zoom at cursor position (cursor-centered zoom)
         * @param cx Screen X cursor position
         * @param cy Screen Y cursor position
         * @param zoomDelta Zoom delta (+1 = zoom in, -1 = zoom out)
         * @param fbW Framebuffer width
         * @param fbH Framebuffer height
         */
        void zoomAt(float cx, float cy, float zoomDelta, int fbW, int fbH) noexcept;

        /**
         * Reset camera to initial state
         */
        void reset() noexcept;

        /**
         * Get current scale
         */
        float scale() const noexcept { return scale_; }

        /**
         * Get world origin (top-left world pixel visible at screen origin)
         */
        glm::vec2 worldOrigin() const noexcept { return glm::vec2(worldOriginX_, worldOriginY_); }

        /**
         * Convert screen coordinates to world coordinates
         * @param sx Screen X
         * @param sy Screen Y
         * @return World coordinates
         */
        glm::vec2 screenToWorld(float sx, float sy) const noexcept;

        /**
         * Convert world coordinates to screen coordinates
         * @param wx World X
         * @param wy World Y
         * @return Screen coordinates
         */
        glm::vec2 worldToScreen(float wx, float wy) const noexcept;

        /**
         * Get orthographic projection matrix (screen space, Y-down)
         * Maps [0, fbW] x [0, fbH] to clip space [-1,1] x [1,-1]
         */
        glm::mat4 ortho(int fbW, int fbH) const noexcept;

        /**
         * Get view matrix (world to screen transformation)
         * Applies scale and translation
         */
        glm::mat4 viewMatrix() const noexcept;

        /**
         * Get combined Model-View-Projection matrix
         * MVP = ortho * view
         */
        glm::mat4 mvp(int fbW, int fbH) const noexcept;

        /**
         * Set world origin (for initial positioning)
         * @param origin World coordinates to show at screen top-left
         */
        void setWorldOrigin(const glm::vec2& origin) noexcept
        {
            worldOriginX_ = origin.x;
            worldOriginY_ = origin.y;
        }

        // Configuration
        static constexpr float kMinScale = 0.25f;
        static constexpr float kMaxScale = 8.0f;
        static constexpr float kZoomSpeed = 0.1f;

    private:
        // World origin: top-left world pixel offset (screen (0,0) maps to this world point)
        float worldOriginX_ = 0.0f;
        float worldOriginY_ = 0.0f;

        // Scale: pixels per world unit (1.0 = 1:1, 2.0 = zoomed in 2x)
        float scale_ = 1.0f;
    };

} // namespace slippygl::render
