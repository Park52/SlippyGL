#pragma once

#include "TileKey.hpp"
#include "../render/Camera2D.hpp"
#include <vector>
#include <cmath>

namespace slippygl::tile
{
    /**
     * Visible tile range for a viewport
     */
    struct VisibleTileRange
    {
        int zoom = 0;
        int minX = 0, maxX = 0;  // Inclusive tile X range
        int minY = 0, maxY = 0;  // Inclusive tile Y range

        /// Total number of visible tiles
        int tileCount() const noexcept
        {
            return (maxX - minX + 1) * (maxY - minY + 1);
        }
    };

    /**
     * Computes visible tile grid from camera and viewport
     */
    class TileGrid
    {
    public:
        /**
         * Compute visible tiles for current camera view
         * @param camera Current camera state
         * @param fbW Framebuffer width
         * @param fbH Framebuffer height
         * @param zoom Tile zoom level to use
         * @param tileSizePx Tile size in pixels (default 256)
         * @return Vector of visible TileKeys
         */
        static std::vector<TileKey> computeVisible(
            const render::Camera2D& camera,
            int fbW, int fbH,
            int zoom,
            int tileSizePx = kTileSizePx)
        {
            const auto range = computeVisibleRange(camera, fbW, fbH, zoom, tileSizePx);
            return rangeToKeys(range);
        }

        /**
         * Compute visible tile range (more efficient for iteration)
         */
        static VisibleTileRange computeVisibleRange(
            const render::Camera2D& camera,
            int fbW, int fbH,
            int zoom,
            int tileSizePx = kTileSizePx)
        {
            VisibleTileRange range;
            range.zoom = zoom;

            // Screen corners to world coordinates
            const glm::vec2 topLeft = camera.screenToWorld(0.0f, 0.0f);
            const glm::vec2 bottomRight = camera.screenToWorld(
                static_cast<float>(fbW), 
                static_cast<float>(fbH)
            );

            // World pixels to tile indices
            const int rawMinX = TileCoord::worldPxToTileIndex(topLeft.x, tileSizePx);
            const int rawMaxX = TileCoord::worldPxToTileIndex(bottomRight.x, tileSizePx);
            const int rawMinY = TileCoord::worldPxToTileIndex(topLeft.y, tileSizePx);
            const int rawMaxY = TileCoord::worldPxToTileIndex(bottomRight.y, tileSizePx);

            // Clamp every index to [0, 2^zoom - 1]. Clamping BOTH ends (not just
            // min-with-0 and max-with-maxIdx) prevents an inverted minX>maxX
            // range, which would silently render zero tiles. (no wrapping yet)
            range.minX = TileCoord::clampTileIndex(rawMinX, zoom);
            range.maxX = TileCoord::clampTileIndex(rawMaxX, zoom);
            range.minY = TileCoord::clampTileIndex(rawMinY, zoom);
            range.maxY = TileCoord::clampTileIndex(rawMaxY, zoom);

            return range;
        }

        /**
         * Convert range to vector of TileKeys
         */
        static std::vector<TileKey> rangeToKeys(const VisibleTileRange& range)
        {
            std::vector<TileKey> keys;
            keys.reserve(range.tileCount());

            for (int y = range.minY; y <= range.maxY; ++y)
            {
                for (int x = range.minX; x <= range.maxX; ++x)
                {
                    keys.emplace_back(range.zoom, x, y);
                }
            }

            return keys;
        }

        /**
         * Calculate world pixel position of a tile's top-left corner
         */
        static glm::vec2 tileWorldPosition(const TileKey& key, int tileSizePx = kTileSizePx)
        {
            return glm::vec2(
                TileCoord::tileIndexToWorldPx(key.x, tileSizePx),
                TileCoord::tileIndexToWorldPx(key.y, tileSizePx)
            );
        }
    };

} // namespace slippygl::tile
