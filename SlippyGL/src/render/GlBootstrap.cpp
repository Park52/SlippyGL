#include "GlBootstrap.hpp"

// GLAD must be included before GLFW to avoid APIENTRY redefinition
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace slippygl::render
{

GlBootstrap::~GlBootstrap()
{
    shutdown();
}

bool GlBootstrap::init(const WindowConfig& cfg)
{
    // Initialize GLFW
    if (!glfwInit()) {
        spdlog::error("GLFW init failed");
        return false;
    }

    // OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    window_ = glfwCreateWindow(cfg.width, cfg.height, cfg.title, nullptr, nullptr);
    if (!window_) {
        spdlog::error("GLFW window creation failed");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    // Load GLAD
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        spdlog::error("GLAD load failed");
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
        return false;
    }

    // Store viewport size
    glfwGetFramebufferSize(window_, &width_, &height_);
    glViewport(0, 0, width_, height_);

    // Enable VSync
    glfwSwapInterval(1);

    spdlog::info("OpenGL {} initialized ({}x{})", 
                 reinterpret_cast<const char*>(glGetString(GL_VERSION)),
                 width_, height_);

    return true;
}

void GlBootstrap::shutdown()
{
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

bool GlBootstrap::shouldClose() const
{
    return window_ ? glfwWindowShouldClose(window_) : true;
}

void GlBootstrap::poll()
{
    glfwPollEvents();

    // Exit on ESC key
    if (window_ && glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }

    // Update viewport size on resize
    if (window_) {
        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        if (w != width_ || h != height_) {
            width_ = w;
            height_ = h;
            glViewport(0, 0, width_, height_);
        }
    }
}

void GlBootstrap::beginFrame(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GlBootstrap::endFrame()
{
    if (window_) {
        glfwSwapBuffers(window_);
    }
}

} // namespace slippygl::render