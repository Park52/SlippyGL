#pragma once

#include <functional>

struct GLFWwindow;

namespace slippygl::render
{
    class Camera2D;

    /**
     * Input state and callback handler for Camera2D
     * Bridges GLFW callbacks to Camera2D operations
     */
    class InputHandler
    {
    public:
        InputHandler() = default;
        ~InputHandler();

        // Non-copyable
        InputHandler(const InputHandler&) = delete;
        InputHandler& operator=(const InputHandler&) = delete;

        /**
         * Attach to GLFW window and camera
         * @param window GLFW window handle
         * @param camera Camera2D to control
         */
        void attach(GLFWwindow* window, Camera2D* camera);

        /**
         * Detach from current window
         */
        void detach();

        /**
         * Check if attached
         */
        bool isAttached() const noexcept { return window_ != nullptr; }

        // For access in static callbacks
        static InputHandler* getHandler(GLFWwindow* window);

    private:
        GLFWwindow* window_ = nullptr;
        Camera2D* camera_ = nullptr;

        // Mouse state
        bool isDragging_ = false;
        double lastMouseX_ = 0.0;
        double lastMouseY_ = 0.0;

        // GLFW callback handlers
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

        // Instance methods called by static callbacks
        void onMouseButton(int button, int action, int mods);
        void onCursorPos(double xpos, double ypos);
        void onScroll(double xoffset, double yoffset);
        void onKey(int key, int scancode, int action, int mods);
    };

} // namespace slippygl::render
