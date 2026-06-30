#include "check.hpp"
#include "render/Camera2D.hpp"
#include "tile/TileGrid.hpp"
#include "tile/TileKey.hpp"

using namespace slippygl;
using namespace slippygl::render;
using namespace slippygl::tile;

// Mirrors the app loop's zoom-level stepping + camera remap.
static int step(Camera2D& cam, int z)
{
    while (cam.scale() > 2.0f && z < 19) { ++z; cam.applyZoomStep(2.0f); }
    while (cam.scale() < 0.5f && z > 0)  { --z; cam.applyZoomStep(0.5f); }
    return z;
}

static void expectValidRange(const Camera2D& cam, int z, int fbW, int fbH)
{
    const auto r = TileGrid::computeVisibleRange(cam, fbW, fbH, z);
    const int maxIdx = (1 << z) - 1;
    CHECK(r.tileCount() > 0);     // never silently empty
    CHECK(r.minX <= r.maxX);      // never inverted
    CHECK(r.minY <= r.maxY);
    CHECK(r.minX >= 0);
    CHECK(r.maxX <= maxIdx);
    CHECK(r.minY >= 0);
    CHECK(r.maxY <= maxIdx);
}

void test_tilegrid()
{
    std::printf("[tilegrid]\n");

    const int fbW = 800, fbH = 600;
    int z = 12;
    Camera2D cam;
    const glm::vec2 wp = tileToWorldPixel(TileKey{ 12, 3492, 1586 }); // Seoul
    cam.setWorldOrigin(glm::vec2(wp.x + 128.0f - fbW / 2.0f, wp.y + 128.0f - fbH / 2.0f));

    // initial
    expectValidRange(cam, z, fbW, fbH);

    // zoom OUT repeatedly: tiles must stay visible (regression: was 0 tiles)
    for (int i = 0; i < 40; ++i) { cam.zoomAt(fbW/2.0f, fbH/2.0f, -1.0f, fbW, fbH); z = step(cam, z); }
    expectValidRange(cam, z, fbW, fbH);
    CHECK(z < 12);

    // zoom IN deeply
    for (int i = 0; i < 120; ++i) { cam.zoomAt(fbW/2.0f, fbH/2.0f, 1.0f, fbW, fbH); z = step(cam, z); }
    expectValidRange(cam, z, fbW, fbH);
    CHECK(z > 12);

    // Explicit inverted-range guard: an origin far past the world bounds at a
    // low zoom must clamp (minX<=maxX), not invert.
    Camera2D off;
    off.setWorldOrigin(glm::vec2(1.0e6f, 1.0e6f));
    const auto r = TileGrid::computeVisibleRange(off, fbW, fbH, 4);
    CHECK(r.minX <= r.maxX);
    CHECK(r.minY <= r.maxY);
}
