#include "check.hpp"
#include "tile/TileKey.hpp"

using namespace slippygl::tile;

void test_tilekey()
{
    std::printf("[tilekey]\n");

    CHECK_EQ(kTileSizePx, 256);

    // world px <-> tile index
    CHECK_EQ(TileCoord::worldPxToTileIndex(0.0f), 0);
    CHECK_EQ(TileCoord::worldPxToTileIndex(255.0f), 0);
    CHECK_EQ(TileCoord::worldPxToTileIndex(256.0f), 1);
    CHECK_EQ(TileCoord::worldPxToTileIndex(-1.0f), -1);  // floor of negative
    CHECK_EQ(TileCoord::tileIndexToWorldPx(0), 0.0f);
    CHECK_EQ(TileCoord::tileIndexToWorldPx(5), 1280.0f);

    // clamp to [0, 2^zoom - 1]
    CHECK_EQ(TileCoord::clampTileIndex(-3, 5), 0);
    CHECK_EQ(TileCoord::clampTileIndex(10, 5), 10);
    CHECK_EQ(TileCoord::clampTileIndex(100, 5), 31);   // 2^5 - 1
    CHECK_EQ(TileCoord::clampTileIndex(0, 0), 0);      // 2^0 - 1 = 0

    // tile -> world pixel (top-left corner)
    const glm::vec2 wp = tileToWorldPixel(TileKey{ 12, 3492, 1586 });
    CHECK_EQ(wp.x, 3492.0f * 256.0f);
    CHECK_EQ(wp.y, 1586.0f * 256.0f);
}
