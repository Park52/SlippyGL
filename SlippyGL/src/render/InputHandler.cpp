#include "InputHandler.hpp"
#include "Camera2D.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <cmath>

namespace slippygl::render
{

InputHandler::~InputHandler()
{
    detach();
}

void InputHandler::attach(GLFWwindow* window, Camera2D* camera)
{
    if (window_ != nullptr) {
        detach();
    }

    window_ = window;
    camera_ = camera;

    if (!window_ || !camera_) {
        spdlog::error("InputHandler: null window or camera");
        return;
    }

    // Store this pointer in window user data for callbacks
    glfwSetWindowUserPointer(window_, this);

    // Set callbacks
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetKeyCallback(window_, keyCallback);

    // Initialize mouse position
    glfwGetCursorPos(window_, &lastMouseX_, &lastMouseY_);

    // Initialize WASD pan timing
    lastUpdateTime_ = glfwGetTime();
    panVelX_ = 0.0f;
    panVelY_ = 0.0f;

    spdlog::info("InputHandler attached");
}

void InputHandler::detach()
{
    if (window_) {
        // Clear callbacks
        glfwSetMouseButtonCallback(window_, nullptr);
        glfwSetCursorPosCallback(window_, nullptr);
        glfwSetScrollCallback(window_, nullptr);
        glfwSetKeyCallback(window_, nullptr);
        glfwSetWindowUserPointer(window_, nullptr);
        
        window_ = nullptr;
        camera_ = nullptr;
        isDragging_ = false;
        panVelX_ = 0.0f;
        panVelY_ = 0.0f;

        spdlog::debug("InputHandler detached");
    }
}

void InputHandler::update()
{
    if (!window_ || !camera_) return;

    // Delta time (computed internally), clamped to avoid jumps on stalls.
    const double now = glfwGetTime();
    float dt = static_cast<float>(now - lastUpdateTime_);
    lastUpdateTime_ = now;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;  // cap at 100ms

    // Target pan direction from key state.
    // W/Up = north (up), S/Down = south, A/Left = west, D/Right = east.
    auto down = [&](int key) { return glfwGetKey(window_, key) == GLFW_PRESS; };
    float tx = 0.0f, ty = 0.0f;
    if (down(GLFW_KEY_W) || down(GLFW_KEY_UP))    ty += 1.0f;
    if (down(GLFW_KEY_S) || down(GLFW_KEY_DOWN))  ty -= 1.0f;
    if (down(GLFW_KEY_A) || down(GLFW_KEY_LEFT))  tx += 1.0f;
    if (down(GLFW_KEY_D) || down(GLFW_KEY_RIGHT)) tx -= 1.0f;

    // Normalize diagonal so it isn't sqrt(2) faster.
    if (tx != 0.0f && ty != 0.0f) {
        constexpr float kInvSqrt2 = 0.70710678f;
        tx *= kInvSqrt2;
        ty *= kInvSqrt2;
    }

    // Target velocity in screen px/sec (camera.pan() divides by scale, so the
    // on-screen pan speed stays constant across zoom levels).
    constexpr float kPanSpeed = 700.0f;
    const float targetVX = tx * kPanSpeed;
    const float targetVY = ty * kPanSpeed;

    // Smoothly approach target velocity (frame-rate independent easing):
    // factor = 1 - exp(-k*dt) gives acceleration when keys are held and
    // deceleration when released.
    constexpr float kSmooth = 12.0f;
    const float a = 1.0f - std::exp(-kSmooth * dt);
    panVelX_ += (targetVX - panVelX_) * a;
    panVelY_ += (targetVY - panVelY_) * a;

    // Apply pan; let tiny residual velocity settle to a clean stop.
    if (std::fabs(panVelX_) > 0.01f || std::fabs(panVelY_) > 0.01f) {
        camera_->pan(panVelX_ * dt, panVelY_ * dt);
    } else {
        panVelX_ = 0.0f;
        panVelY_ = 0.0f;
    }
}

InputHandler* InputHandler::getHandler(GLFWwindow* window)
{
    return static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
}

// Static callback implementations
void InputHandler::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (auto* handler = getHandler(window)) {
        handler->onMouseButton(button, action, mods);
    }
}

void InputHandler::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (auto* handler = getHandler(window)) {
        handler->onCursorPos(xpos, ypos);
    }
}

void InputHandler::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)xoffset; // unused
    if (auto* handler = getHandler(window)) {
        handler->onScroll(xoffset, yoffset);
    }
}

void InputHandler::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (auto* handler = getHandler(window)) {
        handler->onKey(key, scancode, action, mods);
    }
}

// Instance method implementations
void InputHandler::onMouseButton(int button, int action, int mods)
{
    (void)mods; // unused

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging_ = true;
            glfwGetCursorPos(window_, &lastMouseX_, &lastMouseY_);
        } else if (action == GLFW_RELEASE) {
            isDragging_ = false;
        }
    }
}

void InputHandler::onCursorPos(double xpos, double ypos)
{
    if (isDragging_ && camera_) {
        const float dx = static_cast<float>(xpos - lastMouseX_);
        const float dy = static_cast<float>(ypos - lastMouseY_);

        camera_->pan(dx, dy);

        lastMouseX_ = xpos;
        lastMouseY_ = ypos;
    } else {
        // Update position even when not dragging (for zoom cursor position)
        lastMouseX_ = xpos;
        lastMouseY_ = ypos;
    }
}

void InputHandler::onScroll(double xoffset, double yoffset)
{
    (void)xoffset; // unused, horizontal scroll ignored

    if (!camera_ || !window_) return;

    // Get cursor position for cursor-centered zoom
    double cx, cy;
    glfwGetCursorPos(window_, &cx, &cy);

    // Get framebuffer size
    int fbW, fbH;
    glfwGetFramebufferSize(window_, &fbW, &fbH);

    // Apply zoom (positive yoffset = scroll up = zoom in)
    camera_->zoomAt(
        static_cast<float>(cx), 
        static_cast<float>(cy), 
        static_cast<float>(yoffset),
        fbW, fbH
    );
}

void InputHandler::onKey(int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_R && camera_) {
        // Reset camera
        camera_->reset();
        spdlog::info("Camera reset");
    }
    else if (key == GLFW_KEY_F3) {
        // Toggle debug overlay (tile borders + z/x/y)
        debugMode_ = !debugMode_;
        spdlog::info("Debug overlay: {}", debugMode_ ? "ON" : "OFF");
    }
    // Note: ESC handling is done in GlBootstrap::poll()
}

} // namespace slippygl::render
