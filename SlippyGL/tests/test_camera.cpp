#include "check.hpp"
#include "render/Camera2D.hpp"

using namespace slippygl::render;

void test_camera()
{
    std::printf("[camera]\n");

    // applyZoomStep must preserve a point's on-screen position across a level
    // switch. After zoom-in (factor 2), the same geographic point is at world*2.
    {
        Camera2D cam;
        cam.setWorldOrigin(glm::vec2(1000.0f, 2000.0f));   // scale 1.0
        const glm::vec2 wp(1234.0f, 5678.0f);
        const glm::vec2 before = cam.worldToScreen(wp.x, wp.y);

        cam.applyZoomStep(2.0f);
        const glm::vec2 after = cam.worldToScreen(wp.x * 2.0f, wp.y * 2.0f);

        CHECK_NEAR(before.x, after.x, 1e-3);
        CHECK_NEAR(before.y, after.y, 1e-3);
        CHECK_NEAR(cam.scale(), 0.5f, 1e-6);   // scale halved

        // zoom back out restores scale
        cam.applyZoomStep(0.5f);
        CHECK_NEAR(cam.scale(), 1.0f, 1e-6);
    }

    // screenToWorld and worldToScreen are inverses
    {
        Camera2D cam;
        cam.setWorldOrigin(glm::vec2(500.0f, 700.0f));
        const glm::vec2 s = cam.worldToScreen(900.0f, 1300.0f);
        const glm::vec2 w = cam.screenToWorld(s.x, s.y);
        CHECK_NEAR(w.x, 900.0f, 1e-3);
        CHECK_NEAR(w.y, 1300.0f, 1e-3);
    }

    // pan shifts worldOrigin by -delta/scale (scale 1.0 here)
    {
        Camera2D cam;
        const glm::vec2 o0 = cam.worldOrigin();
        cam.pan(10.0f, -20.0f);
        CHECK_NEAR(cam.worldOrigin().x, o0.x - 10.0f, 1e-4);
        CHECK_NEAR(cam.worldOrigin().y, o0.y + 20.0f, 1e-4);
    }
}
