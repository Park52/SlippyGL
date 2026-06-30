#include "check.hpp"
#include "core/TileMath.hpp"

using namespace slippygl::core;

void test_tilemath()
{
    std::printf("[tilemath]\n");

    // Known value: Seoul City Hall (lon 126.9780, lat 37.5665) at zoom 12 -> 3492/1586
    const TileID id = TileMath::lonlatToTileID(126.9780, 37.5665, 12);
    CHECK_EQ(id.z(), 12);
    CHECK_EQ(id.x(), 3492);
    CHECK_EQ(id.y(), 1586);

    // World pixel size doubles per zoom level
    CHECK_EQ(TileMath::worldSizePx(0), 256);
    CHECK_EQ(TileMath::worldSizePx(1), 512);
    CHECK_EQ(TileMath::worldSizePx(12), 256 * 4096);

    // Longitude maps -180..180 -> 0..world; 0 deg = horizontal center
    const double world2 = static_cast<double>(TileMath::worldSizePx(2));
    CHECK_NEAR(TileMath::lonToXpx(-180.0, 2), 0.0, 1e-6);
    CHECK_NEAR(TileMath::lonToXpx(0.0, 2), world2 * 0.5, 1e-6);
    // +180 deg normalizes to -180 (same meridian) -> wraps to the left edge (0)
    CHECK_NEAR(TileMath::lonToXpx(180.0, 2), 0.0, 1e-6);
    // just inside the antimeridian approaches the right edge
    CHECK(TileMath::lonToXpx(179.0, 2) > world2 * 0.99);

    // Latitude 0 deg = vertical center (Web Mercator)
    CHECK_NEAR(TileMath::latToYpx(0.0, 2), world2 * 0.5, 1e-3);
    // Northern latitudes map to smaller Y (closer to top)
    CHECK(TileMath::latToYpx(60.0, 5) < TileMath::latToYpx(0.0, 5));

    // px -> tile index is a floor
    CHECK_EQ(TileMath::pxToTile(255.0), 0);
    CHECK_EQ(TileMath::pxToTile(256.0), 1);
    CHECK_EQ(TileMath::pxToTile(257.0), 1);
}
