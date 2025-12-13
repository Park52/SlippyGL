#include "Camera2D.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace slippygl::render
{

void Camera2D::pan(float dx, float dy) noexcept
{
    // Screen drag translates to world movement (inverse of scale)
    // Moving mouse right (dx > 0) should move the world left (origin increases)
    worldOriginX_ -= dx / scale_;
    worldOriginY_ -= dy / scale_;
}

void Camera2D::zoomAt(float cx, float cy, float zoomDelta, int fbW, int fbH) noexcept
{
    (void)fbW; // unused, but kept for API consistency
    (void)fbH;

    // Get world point under cursor before zoom
    const glm::vec2 worldBefore = screenToWorld(cx, cy);

    // Apply zoom
    const float factor = 1.0f + kZoomSpeed * zoomDelta;
    scale_ *= factor;

    // Clamp scale
    scale_ = std::clamp(scale_, kMinScale, kMaxScale);

    // Get world point under cursor after zoom
    const glm::vec2 worldAfter = screenToWorld(cx, cy);

    // Adjust origin so the same world point stays under cursor
    worldOriginX_ += (worldBefore.x - worldAfter.x);
    worldOriginY_ += (worldBefore.y - worldAfter.y);
}

void Camera2D::reset() noexcept
{
    worldOriginX_ = 0.0f;
    worldOriginY_ = 0.0f;
    scale_ = 1.0f;
}

glm::vec2 Camera2D::screenToWorld(float sx, float sy) const noexcept
{
    // Screen coordinate to world coordinate
    // screen(0,0) -> world(worldOriginX_, worldOriginY_)
    // screen(sx, sy) -> world(worldOriginX_ + sx/scale_, worldOriginY_ + sy/scale_)
    return glm::vec2(
        worldOriginX_ + sx / scale_,
        worldOriginY_ + sy / scale_
    );
}

glm::vec2 Camera2D::worldToScreen(float wx, float wy) const noexcept
{
    // Inverse of screenToWorld
    return glm::vec2(
        (wx - worldOriginX_) * scale_,
        (wy - worldOriginY_) * scale_
    );
}

glm::mat4 Camera2D::ortho(int fbW, int fbH) const noexcept
{
    // Orthographic projection: Y-down coordinate system
    // Maps [0, fbW] x [0, fbH] to clip space [-1,1] x [1,-1]
    // glm::ortho(left, right, bottom, top, near, far)
    // For Y-down: top=0, bottom=fbH
    return glm::ortho(
        0.0f, static_cast<float>(fbW),   // left, right
        static_cast<float>(fbH), 0.0f,   // bottom, top (Y-down)
        -1.0f, 1.0f                       // near, far
    );
}

glm::mat4 Camera2D::viewMatrix() const noexcept
{
    // View matrix: world to screen transformation
    // 1. Translate by -worldOrigin (move world so origin is at camera position)
    // 2. Scale by scale_ (zoom)
    //
    // For a point P_world:
    // P_screen = scale_ * (P_world - worldOrigin)
    //          = scale_ * P_world - scale_ * worldOrigin
    //
    // As a matrix:
    // [scale_  0      0   -scale_*worldOriginX_]
    // [0       scale_ 0   -scale_*worldOriginY_]
    // [0       0      1   0                    ]
    // [0       0      0   1                    ]

    glm::mat4 view(1.0f);
    
    // First translate, then scale (applied in reverse order in matrix form)
    view = glm::scale(view, glm::vec3(scale_, scale_, 1.0f));
    view = glm::translate(view, glm::vec3(-worldOriginX_, -worldOriginY_, 0.0f));
    
    return view;
}

glm::mat4 Camera2D::mvp(int fbW, int fbH) const noexcept
{
    // MVP = Projection * View
    return ortho(fbW, fbH) * viewMatrix();
}

} // namespace slippygl::render
